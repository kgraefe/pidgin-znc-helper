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
#include "internal.h"

#include "timeparser.h"
#include "messageparser.h"

#include <conversation.h>
#include <debug.h>
#include <accountopt.h>

GHashTable *conversations;

void message_parser_init(PurplePlugin *plugin) {
	GList *iter;
	PurplePlugin *prpl;
	PurplePluginProtocolInfo *prpl_info;
	PurpleAccountOption *option;
	
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
}

void message_parser_destroy(void) {
	g_hash_table_destroy(conversations);
}

gboolean writing_chat_msg_cb(PurpleAccount *account, const char *who, char **message, PurpleConversation *conv, PurpleMessageFlags flags) {
	gboolean cancel = FALSE;
	
	static gboolean inuse = FALSE;
	char *pos = NULL;
	time_t stamp;
	
	if(inuse) return FALSE;
	if(!purple_account_get_bool(account, "uses_znc_bouncer", TRUE)) return cancel;
	if(!(flags & PURPLE_MESSAGE_RECV)) return cancel;

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
		pos = g_strrstr(*message, "[");
		
		if(pos != NULL) {
			stamp = get_time(pos);
			if(stamp != 0) {
				*pos = '\0';
				
				inuse = TRUE;
				purple_conv_chat_write(PURPLE_CONV_CHAT(conv), who, *message, flags, stamp);
				inuse = FALSE;
				
				cancel = TRUE;
			}
		} else {
			purple_debug_error(PLUGIN_STATIC_NAME, _("Timestamp could not be interpreted. Please use \"[%%Y-%%m-%%d %%H:%%M:%%S]\" as timestamp format and append it to the message.\n"));
		}
	}
	
	return cancel;
}
