/*
* ZNC Helper
* Copyright (C) 2009 Konrad Gräfe
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

#include "config.h"

/* #define PLUGIN_PREFS_PREFIX "/plugins/core/talklikeapirate" */

#ifndef PURPLE_PLUGINS
#define PURPLE_PLUGINS
#endif

#include "internal.h"

#include <plugin.h>
#include <debug.h>
#include <version.h>
#include <conversation.h>
#include <glib.h>
#include <accountopt.h>

PurplePlugin *plugin;

GHashTable *conversations;

static gboolean writing_chat_msg_cb(PurpleAccount *account, const char *who, char **message, PurpleConversation *conv, PurpleMessageFlags flags) {
	gboolean cancel = FALSE;
	
	static gboolean inuse = FALSE;
	char **tokens = NULL;
	char *strtime = NULL;
	time_t stamp;
	
	if(inuse) return FALSE;
	if(!purple_account_get_bool(account, "uses_znc_bouncer", TRUE)) return cancel;
	if(!(flags & PURPLE_MESSAGE_RECV)) return cancel;
	
	purple_debug_info(PLUGIN_STATIC_NAME, "message: %s\n", *message);

	if(purple_utf8_strcasecmp(who, "***") == 0) {
		if(purple_utf8_strcasecmp(*message, "Buffer Playback...") == 0) {
			g_hash_table_insert(conversations, conv, GINT_TO_POINTER(1)); /* GINT_TO_POINTER(1) to  distinguish from NULL */
			
			inuse = TRUE;
			purple_conv_chat_write(PURPLE_CONV_CHAT(conv), who, _("Buffer Playback..."), flags|PURPLE_MESSAGE_SYSTEM, time(NULL));
			inuse = FALSE;
			
			cancel = TRUE;
		} else if(purple_utf8_strcasecmp(*message, "Playback Complete.") == 0) {
			g_hash_table_remove(conversations, conv);
			
			inuse = TRUE;
			purple_conv_chat_write(PURPLE_CONV_CHAT(conv), who, _("Playback Complete."), flags|PURPLE_MESSAGE_SYSTEM, time(NULL));
			inuse = FALSE;
			
			cancel = TRUE;
		}
	} else if(GPOINTER_TO_INT(g_hash_table_lookup(conversations, conv)) == 1)  {
		tokens = g_strsplit(*message, "] ", 2);
		
		if(g_strv_length(tokens)==2) {
			strtime = g_strstrip(g_strdelimit(tokens[0], "[]", ' '));
			
			stamp = purple_str_to_time(strtime, FALSE, NULL, NULL, NULL);
			
			if(stamp != 0) {
				purple_conv_chat_write(PURPLE_CONV_CHAT(conv), who, tokens[1], flags, stamp);
				
				cancel = TRUE;
			} else {
				purple_debug_error(PLUGIN_STATIC_NAME, _("Timestamp could not be interpreted. Please use \"[%%Y-%%m-%%dT%%H:%%M:%%S]\" as timestamp format and prepend it to the message.\n"));
			}
		}
		
		g_strfreev(tokens);
	}
	
	return cancel;
}

static gboolean plugin_load(PurplePlugin *_plugin) {
	GList *iter;
	PurplePlugin *prpl;
	PurplePluginProtocolInfo *prpl_info;
	PurpleAccountOption *option;
	
	plugin = _plugin;
	
	conversations = g_hash_table_new(NULL, NULL);
	
	/* Allen IRC-Accounts die ZNC-Option anhängen*/
	for (iter = purple_plugins_get_protocols(); iter; iter = iter->next) {
		prpl = iter->data;
		
		if(prpl && prpl->info) {
			prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);
			if(prpl_info && prpl->info->id && (purple_utf8_strcasecmp(prpl->info->id, "prpl-irc")==0)) {
				option = purple_account_option_bool_new(_("Uses ZNC Bouncer"), "uses_znc_bouncer", FALSE);
				prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
			}
		}
	}
	
	purple_signal_connect(purple_conversations_get_handle(), "writing-chat-msg", plugin, PURPLE_CALLBACK(writing_chat_msg_cb), NULL);
	
	return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin) {
	g_hash_table_destroy(conversations);
	
	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,		/**< type           */
	NULL,						/**< ui_requirement */
	0,							/**< flags          */
	NULL,						/**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,	/**< priority       */

	PLUGIN_ID,					/**< id             */
	NULL,						/**< name           */
	PLUGIN_VERSION,				/**< version        */
	NULL,						/**  summary        */
				
	NULL,						/**  description    */
	PLUGIN_AUTHOR,				/**< author         */
	PLUGIN_WEBSITE,				/**< homepage       */

	plugin_load,				/**< load           */
	plugin_unload,				/**< unload         */
	NULL,						/**< destroy        */

	NULL,						/**< ui_info        */
	NULL,						/**< extra_info     */
	NULL,				/**< prefs_info     */
	NULL,						/**< actions        */
	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
        bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

        info.name        = _("ZNC Helper");
        info.summary     = _("ZNC Helper");
        info.description = _("ZNC Helper");
}

PURPLE_INIT_PLUGIN(plugin, init_plugin, info)
