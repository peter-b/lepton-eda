/* gEDA - GPL Electronic Design Automation
 * libgeda - gEDA's library - Scheme API
 * Copyright (C) 2016  Peter Brett <peter@peter-b.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */

/*!
 * \file scheme_log.c
 * \brief Scheme API logging support
 */

#include <config.h>

#include "libgeda_priv.h"
#include "libgedaguile_priv.h"

SCM_SYMBOL(error_sym, "error");
SCM_SYMBOL(critical_sym, "critical");
SCM_SYMBOL(warning_sym, "warning");
SCM_SYMBOL(message_sym, "message");
SCM_SYMBOL(info_sym, "info");
SCM_SYMBOL(debug_sym, "debug");


/*! \brief Convert a Scheme symbol to log level flags.
 * \par Function Description
 * Helper function to construct a #GLogLevelFlags value from a Scheme
 * symbol.
 */
static GLogLevelFlags
decode_level (SCM level_s)
{
	if (level_s == error_sym)    return (G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL);
	if (level_s == critical_sym) return G_LOG_LEVEL_CRITICAL;
	if (level_s == warning_sym)  return G_LOG_LEVEL_WARNING;
	if (level_s == message_sym)  return G_LOG_LEVEL_MESSAGE;
	if (level_s == info_sym)     return G_LOG_LEVEL_INFO;
	if (level_s == debug_sym)    return G_LOG_LEVEL_DEBUG;

	g_return_val_if_reached(G_LOG_LEVEL_MESSAGE);
}

/*! \brief Convert log level flags to a Scheme symbol.
 * \par Function Description
 * Helper function to convert a #GLogLevelFlags to a Scheme symbol.
 */
static SCM
encode_level (GLogLevelFlags level)
{
	switch (level & G_LOG_LEVEL_MASK) {
	case G_LOG_LEVEL_ERROR:    return error_sym;
	case G_LOG_LEVEL_CRITICAL: return critical_sym;
	case G_LOG_LEVEL_WARNING:  return warning_sym;
	case G_LOG_LEVEL_INFO:     return info_sym;
	case G_LOG_LEVEL_DEBUG:    return debug_sym;
	default:                   return message_sym;
	}

	g_warn_if_reached();
	return message_sym;
}

/*! \brief Dispatch a C log handler from Scheme values.
 * \par Function Description
 * Helper function used by default_log_handler(),
 * dispatch_log_handler_closure(), and edascm_c_log_buffer_foreach().
 */
static void
dispatch_log_handler(SCM domain_s,
                     SCM level_s,
                     SCM message_s,
                     GLogFunc callback,
                     gpointer user_data)
{
	scm_dynwind_begin(0);

	char *domain = NULL;
	if (scm_is_string(domain_s)) {
		scm_to_utf8_string(domain_s);
		scm_dynwind_free(domain);
	}

	GLogLevelFlags level = decode_level(level_s);

	char *message = scm_to_utf8_string(message_s);
	scm_dynwind_free(message);

	callback(domain, level, message, user_data);

	scm_dynwind_end();
}

/*! \brief Log handling closure function.
 * \par Function Description
 * Used by edascm_c_install_log_handler() to construct a Scheme
 * closure.
 */
static SCM
dispatch_log_handler_closure(SCM message_info_s,
                             gpointer user_data)
{
	GLogFunc callback = (GLogFunc) user_data;
	SCM domain_s = scm_car(message_info_s);
	SCM level_s = scm_cadr(message_info_s);
	SCM message_s = scm_caddr(message_info_s);
	dispatch_log_handler(domain_s, level_s, message_s, callback, NULL);

	return SCM_UNDEFINED;
}

/*! \brief Pass log messages into Scheme log handling logic
 * \par Function Description
 * This function is installed as the GLib default log handler.  It
 * passes all log messages into the Scheme log handling logic.
 *
 * \see edascm_init_log().
 */
static void
handle_log(const gchar *domain,
           GLogLevelFlags level,
           const gchar *message,
           gpointer unused)
{
	/* Marshal values into Scheme */
	SCM domain_s = domain ? scm_from_utf8_string(domain) : SCM_BOOL_F;
	SCM level_s = encode_level(level);
	SCM message_s = scm_from_utf8_string(message ? message : "");

	/* Get the log handler */
	SCM default_log_handler_proc_s =
		scm_variable_ref(scm_c_public_variable("geda log",
		                                       "default-log-handler!"));

	/* Call it */
	scm_call_3(default_log_handler_proc_s, domain_s, level_s, message_s);
}

/* ================================================================
 * Functions for use from C
 * ================================================================ */

/*! \brief Start logging to the default file for a tool
 * \par Function Description
 * Open a new log file for \a toolname in the per-user log directory,
 * and initialise it with the contents of the log buffer.  All
 * subsequent log messages are sent to the log file.
 *
 * \param toolname  the name of the tool for which to open a log file
 */
void
edascm_c_begin_default_log_file(const gchar *toolname)
{
	SCM begin_file_proc_s;

	g_return_if_fail(NULL != toolname);

	/* Get the begin-default-log-file! procedure */
	begin_file_proc_s =
		scm_variable_ref(scm_c_public_variable("geda log",
		                                       "begin-default-log-file!"));

	scm_call_1(begin_file_proc_s, scm_from_utf8_string(toolname));
}

/*! \brief Apply a function to each message in the log buffer.
 * \par Function Description
 * Call \a callback for each entry in the current log buffer, from
 * oldest to newest.
 *
 * \param callback  a #GLogFunc to be called for each message
 * \param user_data extra data to be passed to \a callback
 */
void
edascm_c_log_buffer_foreach(GLogFunc callback,
                            gpointer user_data)
{
	SCM buffered_log_messages_proc_s;
	SCM buffer_s;

	g_return_if_fail(NULL != callback);

	/* Get the buffered-log-messages procedure */
	buffered_log_messages_proc_s =
		scm_variable_ref(scm_c_public_variable("geda log",
		                                       "buffered-log-messages"));

	/* Call it to get buffer contents */
	buffer_s = scm_call_0(buffered_log_messages_proc_s);

	/* Loop over the buffer, calling the callback for each entry */
	SCM curr_s;
	for (curr_s = buffer_s; curr_s != SCM_EOL; curr_s = scm_cdr(curr_s)) {
		SCM entry_s = scm_car(curr_s);

		/* Skip the timestamp */
		entry_s = scm_cdr(entry_s);

		dispatch_log_handler(scm_car(entry_s),
		                     scm_cadr(entry_s),
		                     scm_caddr(entry_s),
		                     callback, user_data);
	}
}

/*! \brief Install a log handler
 * \par Function Description
 * Install \a callback to be called whenever a new log message is
 * sent.
 *
 * \param callback  Function to be called for each log message.
 */
void
edascm_c_install_log_handler(GLogFunc callback)
{
	SCM closure_s = edascm_c_make_closure(dispatch_log_handler_closure,
	                                      callback);

	SCM install_log_handler_proc =
		scm_variable_ref(scm_c_public_variable("geda log",
		                                       "install-log-handler!"));

	scm_call_1(install_log_handler_proc, closure_s);
}

/* ================================================================
 * Functions for use from Scheme
 * ================================================================ */

/*! \brief Log a message.
 * \par Function Description
 * Add a message to the message log.  The \a domain_s should normally
 * be SCM_BOOL_F, and the \a message_s should almost always be
 * translated for all log levels other than "debug".  The \a level_s
 * should be one of the symbols "error", "critical", "message", "info"
 * or "debug".
 *
 * \note Scheme API: Implements the \%log! procedure in the (geda core
 * log) module.
 *
 * \param domain_s  The log domain, as a string, or SCM_BOOL_F.
 * \param level_s   The log level, as a symbol.
 * \param message_s The log message, as a string.
 *
 * \return undefined.
 */
SCM_DEFINE (log_x, "%log!", 3, 0, 0,
            (SCM domain_s, SCM level_s, SCM message_s),
            "Emit a log message.")
{
	SCM_ASSERT (scm_is_false(domain_s) || scm_is_string(domain_s), domain_s,
	            SCM_ARG1, s_log_x);
	SCM_ASSERT (scm_is_symbol(level_s), level_s,
	            SCM_ARG2, s_log_x);
	SCM_ASSERT (scm_is_string(message_s), message_s,
	            SCM_ARG3, s_log_x);

	scm_dynwind_begin(0);
	gchar *domain = NULL;
	if (domain) {
		scm_to_utf8_string(domain_s);
		scm_dynwind_free(domain);
	}
	gchar *message = scm_to_utf8_string(message_s);
	scm_dynwind_free(message);
	GLogLevelFlags level = decode_level(level_s);

	g_log(domain, level, message);

	scm_dynwind_end();

	return SCM_UNSPECIFIED;
}

/*! \brief The default log handler.
 * \par Function Description
 * Wraps the GLib default log handler.
 *
 * \note Scheme API: Implements the \%default-log-handler procedure in
 * the (geda core log) module.
 *
 * \param domain_s  The log domain, as a string, or SCM_BOOL_F.
 * \param level_s   The log level, as a symbol.
 * \param message_s The log message, as a string.
 *
 * \return undefined.
 */
SCM_DEFINE (default_log_handler, "%default-log-handler", 3, 0, 0,
            (SCM domain_s, SCM level_s, SCM message_s),
            "The default log message handler.")
{
	SCM_ASSERT (scm_is_false(domain_s) || scm_is_string(domain_s),
	            domain_s, SCM_ARG1, s_default_log_handler);
	SCM_ASSERT (scm_is_symbol(level_s), level_s,
	            SCM_ARG2, s_default_log_handler);
	SCM_ASSERT (scm_is_string(message_s), message_s,
	            SCM_ARG3, s_default_log_handler);

	dispatch_log_handler(domain_s, level_s, message_s,
	                     g_log_default_handler, NULL);

	return SCM_UNDEFINED;
}

/* ================================================================
 * Initialization
 * ================================================================ */

/*!
 * \brief Create the (geda core log) Scheme module.
 * \par Function Description
 * Defines procedures in the (geda core log) module.  The module can
 * be accessed using (use-modules (geda core log)).
 */
static void
init_module_geda_core_log ()
{
	/* Register the functions and symbols */
	#include "scheme_log.x"

	/* Add them to the module's public definitions */
	scm_c_export (s_log_x, s_default_log_handler,
	              NULL);
}

/*!
 * \brief Initialise the basic gEDA logging procedures
 * \par Function Description
 *
 * Registers some core Scheme procedures for logging support.  Should
 * only be called by edascm_init().
 */
void
edascm_init_log ()
{
	/* Define the (geda core log) module */
	scm_c_define_module ("geda core log",
	                     init_module_geda_core_log,
	                     NULL);

	/* Forcibly load the (geda log) module */
	scm_c_resolve_module("geda log");


	/* Hook in the Scheme library as the GLib log handler */
	g_log_set_default_handler(handle_log, NULL);
}
