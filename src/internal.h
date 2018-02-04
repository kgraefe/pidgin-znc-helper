/*
 * Pidgin ZNC Helper
 * Copyright (C) 2009-2016 Konrad Gräfe
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#ifndef INTERNAL_H
#define INTERNAL_H

#include "config.h"

#include <glib.h>
#include <libpurple/debug.h>

#define debug(fmt, ...) \
	purple_debug_info(PLUGIN_STATIC_NAME, fmt, ##__VA_ARGS__)
#define error(fmt, ...) \
	purple_debug_error(PLUGIN_STATIC_NAME, fmt, ##__VA_ARGS__)

#if GLIB_CHECK_VERSION(2,4,0)
#include <glib/gi18n-lib.h>
#else
#include <locale.h>
#include <libintl.h>
#define _(String) dgettext (GETTEXT_PACKAGE, String)
#define Q_(String) g_strip_context ((String), dgettext (GETTEXT_PACKAGE, String))
#ifdef gettext_noop
#define N_(String) gettext_noop (String)
#else
#define N_(String) (String)
#endif
#endif

/* This works around the lack of G_GNUC_NULL_TERMINATED in old glib and the
 * lack of the NULL sentinel in GCC older than 4.0.0 and non-GCC compilers */
#ifndef G_GNUC_NULL_TERMINATED
#  if     __GNUC__ >= 4
#    define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#  else
#    define G_GNUC_NULL_TERMINATED
#  endif
#endif

#if GLIB_CHECK_VERSION(2,6,0)
# include <glib/gstdio.h>
#endif /* GLIB_CHECK_VERSION(2,6,0) */

#ifdef _WIN32
# include <win32dep.h>
#endif /* _WIN32 */

#if !GLIB_CHECK_VERSION(2,6,0)
# define g_freopen freopen
# define g_fopen fopen
# define g_rmdir rmdir
# define g_remove remove
# define g_unlink unlink
# define g_lstat lstat
# define g_stat stat
# define g_mkdir mkdir
# define g_rename rename
# define g_open open
#endif /* !GLIB_CHECK_VERSION(2,6,0) */

#endif
