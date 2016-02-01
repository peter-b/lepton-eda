/* gEDA - GPL Electronic Design Automation
 * libgeda - gEDA's library
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <config.h>

#include "libgeda_priv.h"
#include "libgeda/libgedaguile.h"

/*! Default setting for log update callback function. */
void (*x_log_update_func)(const gchar *, GLogLevelFlags, const gchar *) = NULL;

/* ================================================================
 * General logging support for tools
 * ================================================================ */

static void
s_log_update_func__callback(const char *domain,
                            GLogLevelFlags level,
                            const char *message,
                            gpointer unused)
{
	if (x_log_update_func)
		x_log_update_func(domain, level, message);
}

/*! Default setting for log enable. */
int do_logging = TRUE;

/*! \brief Initialize libgeda logging feature.
 *  \par Function Description
 */
void s_log_init (const gchar *prefix)
{
	if (do_logging)
		edascm_c_begin_default_log_file(prefix);

	/* Insert the x_log_update_func() as a log handler */
	edascm_c_install_log_handler(s_log_update_func__callback);
}

/*!
 * \brief Terminates the logging of messages.
 * \bug Doesn't currently do anything.
 */
void s_log_close (void)
{
}

/* ================================================================
 * Support functions for gschem logging
 * ================================================================ */

/*!
 * \brief Accumulates log messages into a GString.
 * \par Function Description
 * Used to implement s_log_read().
 */
static void
s_log_read__callback(const gchar *domain,
                     GLogLevelFlags flags,
                     const gchar *message,
                     gpointer user_data)
{
	GString *str = (GString *) user_data;
	g_string_append(str, message);
	/* Ensure that messages are newline-separated */
	if (message[strlen(message) - 1] != '\n')
		g_string_append_c(str, '\n');
}

/*!
 * \brief Returns the contents of the log buffer as a string.
 * \par Function Description
 * Returns a string containing the messages in the current log buffer.
 * N.b. The domain and priority level information is lost.
 *
 * The returned string should be freed when no longer needed using
 * g_free().
 *
 * \return Newly-allocated string.
 *
 * \bug This functions exist to support populating the gschem log
 * window when it's opened.  Ideally gschem should just look at the
 * log buffer directly, and make use of the cached priority (and
 * possible level) information.
 */
gchar *s_log_read (void)
{
	GString *result = g_string_new(NULL);
	edascm_c_log_buffer_foreach(s_log_read__callback, result);
	return g_string_free(result, FALSE);
}
