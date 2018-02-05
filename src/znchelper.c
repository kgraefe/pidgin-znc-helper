/* Copyright (C) 2009-2018 Konrad Gräfe <konradgraefe@aol.com>
 *
 * This software may be modified and distributed under the terms
 * of the GPLv2 license. See the COPYING file for details.
 */

#include "config.h"

#ifndef PURPLE_PLUGINS
#define PURPLE_PLUGINS
#endif

#include "internal.h"

#include <gtkplugin.h>
#include <debug.h>
#include <version.h>

#include "messageparser.h"
#include "prefs.h"
#include "queryfix.h"

static gboolean plugin_load(PurplePlugin *plugin) {
	message_parser_init();
	query_fix_init(plugin);

	return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin) {
	message_parser_uninit();
	query_fix_uninit(plugin);

	return TRUE;
}

static PidginPluginUiInfo ui_info = {
	get_pref_frame,
	0,   /* page_num (Reserved) */
	/* Padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,     /**< type           */
	PIDGIN_PLUGIN_TYPE,         /**< ui_requirement */
	0,                          /**< flags          */
	NULL,                       /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,    /**< priority       */

	PLUGIN_ID,                  /**< id             */
	NULL,                       /**< name           */
	PLUGIN_VERSION,             /**< version        */
	NULL,                       /**  summary        */
	NULL,                       /**  description    */
	PLUGIN_AUTHOR,              /**< author         */
	PLUGIN_WEBSITE,             /**< homepage       */

	plugin_load,                /**< load           */
	plugin_unload,              /**< unload         */
	NULL,                       /**< destroy        */

	&ui_info,                   /**< ui_info        */
	NULL,                       /**< extra_info     */
	NULL,                       /**< prefs_info     */
	NULL,                       /**< actions        */
	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void init_plugin(PurplePlugin *plugin) {
	const char *str = "ZNC Helper";
	gchar *plugins_locale_dir;
	
#ifdef ENABLE_NLS
	plugins_locale_dir = g_build_filename(purple_user_dir(), "locale", NULL);

	bindtextdomain(GETTEXT_PACKAGE, plugins_locale_dir);
	if(str == _(str)) {
		bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	}
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	g_free(plugins_locale_dir);
#endif /* ENABLE_NLS */

	info.name        = _("ZNC Helper");
	info.summary     = _("Pidgin ZNC Helper parses IRC bouncer timestamps and displays them as normal timestamps.");
	info.description = _(
        "Pidgin ZNC Helper parses IRC bouncer timestamps and displays them as normal timestamps.\n\n"
        "Therefore, the ZNC needs timestamp needs append the timestamp in the the following format:\n"
		"[%Y-%m-%d %H:%M:%S]\n"
    );
		
	purple_prefs_add_none(PLUGIN_PREFS_PREFIX);
	purple_prefs_add_int(PLUGIN_PREFS_PREFIX "/offset", 0);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)
