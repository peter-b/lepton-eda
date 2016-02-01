;; gEDA - GPL Electronic Design Automation
;; libgeda - gEDA's library - Scheme API
;; Copyright (C) 2016 Peter Brett <peter@peter-b.co.uk>
;;
;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2 of the License, or
;; (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
;;

(define-module (geda log)
  #:use-module (geda core log)
  #:use-module (geda core gettext)

  #:use-module (geda os)
  #:use-module (ice-9 format)
  #:use-module (ice-9 ftw)
  #:use-module (ice-9 match)
  #:use-module (ice-9 regex)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-19)
  #:use-module (srfi srfi-26))

;; ================================================================
;; Logging messages
;; ================================================================

#|
Function::

  log! level message [format-args]

Log a new message with the specified log level and contents.

The LEVEL should describe the log level -- for example, one of the
symbols "message", "warning", "critical", "error", "info" or
"debug".  "error"-level messages are fatal.

A newline character is automatically appended to the message.
|#
(define-public (log! level message . format-args)
  (let ((formatted (apply format #f message format-args)))
    (%log! #f level (string-append formatted))))

;; ================================================================
;; Handling log messages
;; ================================================================

(define log-handlers (list %default-log-handler))

#|
Function:

  default-log-handler! domain level message

Handle a log message with the specified DOMAIN, LEVEL and MESSAGE, by
passing it through a series of filters and then to each of the
installed log handlers.

There isn't a good reason to call this directly -- you almost always
want to call log! instead.
|#
(define-public (default-log-handler! domain level message)
  (define (filter message-info)
    (fold apply message-info log-filters))
  (define (handle message-info)
    (for-each (cut apply <> message-info) log-handlers))
  (handle (filter (list domain level message))))

#|
Function::

  install-log-handler! handler

Add handler to the list of procedures that should receive message
info.  Each handler is called like::

  handler domain level message

All handlers receive the same argument list, and the return values are
discarded.
|#
(define-public (install-log-handler! handler-proc)
  (set! log-handlers (cons handler-proc log-handlers)))

;; ================================================================
;; Log message buffering
;; ================================================================
;;
;; We include a buffer for logging for use by gschem's log window.  It
;; preserves at least the last 1024 log messages, to allow a new
;; gschem instance to populate its log area with all of the log info,
;; including timestamps, priorities, etc.
;;
;; This is a functional implementation; the buffer state is a list in
;; the form (COUNT HEAD TAIL).  HEAD and TAIL are lists; COUNT is the
;; length of HEAD.  When HEAD gets to BUFFER-SIZE, the TAIL is
;; replaced by the reverse of HEAD, HEAD is replaced by an empty list,
;; and COUNT is reset to 0.
;;
;; buffered-log-messages returns the current buffer contents.  HEAD is
;; reversed onto TAIL, and the resulting list becomes the new TAIL
;; (and the return value of buffered-log-messages).

;; Almost certainly big enough for any practical set of log messages!
(define BUFFER-SIZE 1024)
;; Start off with an empty buffer
(define current-buffer '(0 () ()))

;; Redefine reverse so that it works like reverse!
(define* (reverse lst #:optional (newtail '()))
   (fold cons newtail lst))

(define (set-buffer! count head tail)
  (set! current-buffer (list count head tail)))

;;;; buffer-message! ARGS...
;;
;; Append the message info in ARGS to the ring buffer, with a
;; timestamp.
(define (buffer-message! . message-info)
  (define now (current-time))
  (define info (cons now message-info))
  (match current-buffer
         ((count head tail)
          (if (>= count BUFFER-SIZE)
              (set-buffer! 1 (list info) (reverse head))
              (set-buffer! (1+ count) (cons info head) tail)))))

;;;; buffer-fetch!
;;
;; Return the current contents of the message ring buffer as a list.
;; Each element of the return value is a list of message info.  Each
;; message info list begins with a timestamp, followed by the other
;; arguments that were passed to buffer-message!.
(define (buffer-fetch!)
  (match current-buffer
         ((count head tail)
          (let ((new-tail (reverse head tail)))
            (set-buffer! 0 '() new-tail)
            new-tail))))

(define-public buffered-log-messages buffer-fetch!)

;; ================================================================
;; Logging to files
;; ================================================================

;; ----------------------------------------------------------------
;; Support functions
;; ----------------------------------------------------------------

;;;; begin-log-port! port
;;
;; Start sending log messages to port.  First, flush the contents of
;; the log buffer to port, and then install a log handler that sends
;; all subsequently-received log messages to port.
(define (begin-log-port! port path)
  ;; Closure used for writing log messages
  (define (write-log! time message-info)
    (write (cons (date->string (time-utc->date time) "~4")
                 message-info)
           port)
    (newline port))

  ;; Helper function for writing log messages from the log buffer
  (define (write-from-buffer! buffer-entry)
    (match buffer-entry
           (((? time? time) . info) (write-log! time info))))

  ;; Flush the log buffer to the new log port
  ;(for-each write-from-buffer! (buffered-log-messages))

  ;; Ensure all future log messages are sent to port
  (install-log-handler! (lambda args (write-log! (current-time) args)))

  (log! 'debug (_ "Started logging to ~s") path))

;; ----------------------------------------------------------------
;; Explicitly-named log files
;; ----------------------------------------------------------------

#|
Function::

  begin-log-file! filename

Begin logging to FILENAME, initialising it with the current contents
of the log buffer.  If FILENAME already exists, log messages are
appended to it.
|#
(define-public (begin-log-file! filename)
  (begin-log-port! (open-file filename "al" #:encoding "utf-8") filename))

;; ----------------------------------------------------------------
;; Default (per-tool) log files
;; ----------------------------------------------------------------

(define MAX_LOG_FILE_TRIES 5)

#|
Function::

  user-log-dir

Returns the directory in which user logs are stored.
|#
;; FIXME This should probably be located in a suitable cache
;; directory.
(define user-log-dir (const (build-path (user-config-dir) "logs")))

;;;; ensure-user-log-dir!
;;
;; Ensure the per-user log directory exists, by creating it (and its
;; parent directories) if necessary.
(define (ensure-user-log-dir!)
  (define (ensure-dir! dir)
    (false-if-exception
     (or (file-exists?     dir)
         (and (ensure-dir! (dirname dir))
              (mkdir dir)
              #t))))
  (ensure-dir! (user-log-dir)))

#|
Function::

  begin-default-log-file! toolname

Begin logging to a new, uniquely-named log file for TOOLNAME in the
per-user log directory, initialising it with the current contents of
the log buffer.  Returns the log filename, or #f if it wasn't possible
to open a suitable log file.
|#
(define-public (begin-default-log-file! toolname)
  ;; log files for the toolname should begin with this prefix...
  (define prefix
    (let ((day (date->string
                (time-utc->date (current-time)) "~Y~m~d")))
      (string-append toolname "-" day)))

  ;; ...and should match this regular expression
  (define log-regexp (make-regexp (format #f "^~a-([0-9]+).log$" prefix)))

  ;; Given a path, return a number if it's a log file for toolname,
  ;; and #f otherwise.
  (define (log-number path)
    (let* ((name (basename path))
           (match (regexp-exec log-regexp name)))
      (false-if-exception (string->number (match:substring match 1)))))

  ;; Try to find the largest number from the log files opened today
  ;; for the specified tool
  (define (max-log-num)
    (define enter? (const #t)) ; Enter all directories
    (define id (match-lambda* ((_ ... result) result))) ; Do nothing
    (define leaf ; Process a filename
      (match-lambda*
       ((path stat result)
        (max (or (log-number path) 0)
             result))))

    (file-system-fold enter? leaf id id id id
                      0 (user-log-dir)))

  ;; Try to open filename, failing if it already exists
  ;; FIXME this might not work on Windows
  (define (open-log-exclusively! filename)
     (false-if-exception
     (open filename (logior O_CREAT O_WRONLY O_EXCL))))

  ;; Open an output port for a newly-created file, with a suitable
  ;; unused sequence number
  (ensure-user-log-dir!)
  (let loop ((n (max-log-num))
             (tries 0))
    (let* ((filename (format #f "~a-~a.log" prefix n))
           (path (build-path (user-log-dir) filename))
           (port (open-log-exclusively! path)))
      (cond

       ;; If the port was opened successfully, install it as a log
       ;; port.
       ((port? port)
        (begin (set-port-encoding! port "utf-8")
               (begin-log-port! port path)
               path))

       ;; If the port couldn't be opened after the maximum number of retries,
       ;; generate a warning message and return #f
       ((>= tries MAX_LOG_FILE_TRIES)
        (begin
          (log! 'warning
                (_ "Failed to open default ~0@*~s logfile after ~1@*~s tries")
                toolname tries)
          #f))

       ;; Otherwise, try again.
       (else
        (loop (1+ n) (1+ tries)))))))

;; ================================================================
;; Initialization
;; ================================================================

;; Ensure all messages are buffered by default.
(install-log-handler! buffer-message!)
