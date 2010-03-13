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
#include "zncconversation.h"

#include <conversation.h>
#include <debug.h>
#include <accountopt.h>
#include <gtkconv.h>
#include <signals.h>

PurplePlugin *plugin;
GHashTable *conversations;

/* copied from Pidgin */
static void pidgin_conv_calculate_newday(PidginConversation *gtkconv, time_t mtime) {
	struct tm *tm = localtime(&mtime);

	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
	tm->tm_mday++;

	gtkconv->newday = mktime(tm);
}

static gchar *conversation_timestamp_cb(PurpleConversation *conv, time_t mtime, gboolean _show_date) {
	PidginConversation *gtkconv;
	gchar *mdate;
	struct tm tm_msg, tm_now;
	time_t tnow;
	const char *tmp;
	gboolean show_date;
	
	gtkconv = PIDGIN_CONVERSATION(conv);
	
	time(&tnow);
	localtime_r(&tnow, &tm_now);
	localtime_r(&mtime, &tm_msg);
	
	purple_debug_info(PLUGIN_STATIC_NAME, "gtkconv->newday:  %i\n", (int)gtkconv->newday);
	
	/* First message in playback */
	if (gtkconv->newday == (-1)) {
		if((tm_msg.tm_year != tm_now.tm_year) || (tm_msg.tm_mon != tm_now.tm_mon) || (tm_msg.tm_mday != tm_now.tm_mday)) {
			show_date = TRUE;
		} else {
			show_date = FALSE;
		}
		
		pidgin_conv_calculate_newday(gtkconv, mtime);
	} else {
		show_date = (mtime >= gtkconv->newday);
	}
	
	if (show_date) {
		tmp = purple_date_format_long(&tm_msg);
	} else {
		tmp = purple_time_format(&tm_msg);
	}
	mdate = g_strdup_printf("(%s)", tmp);
	
	return mdate;
}

static gboolean writing_msg_cb(PurpleAccount *account, const char *who, char **message, PurpleConversation *conv, PurpleMessageFlags flags) {
	ZNCConversation *zncconv = NULL;
	PidginConversation *gtkconv;
	gboolean cancel = FALSE;
	
	static gboolean inuse = FALSE;
	char *pos = NULL;
	time_t stamp;

	
	if(inuse) return FALSE;
	if(!purple_account_get_bool(account, "uses_znc_bouncer", TRUE)) return cancel;
	if(!(flags & PURPLE_MESSAGE_RECV)) return cancel;
	
	gtkconv = PIDGIN_CONVERSATION(conv);

	purple_debug_info(PLUGIN_STATIC_NAME, "*message: %s\n", *message);

	if(purple_utf8_strcasecmp(who, "***") == 0) {
		if(purple_utf8_strcasecmp(*message, "Buffer Playback...") == 0) {
			zncconv = znc_conversation_new(conv);
			g_hash_table_insert(conversations, conv, zncconv);
			
			inuse = TRUE;
			purple_conv_chat_write(PURPLE_CONV_CHAT(conv), who, _("Buffer Playback..."), flags|PURPLE_MESSAGE_SYSTEM, time(NULL));
			
			gtkconv->newday = (-1);
			
			inuse = FALSE;
			
			cancel = TRUE;
		} else if(purple_utf8_strcasecmp(*message, "Playback Complete.") == 0) {
			zncconv = g_hash_table_lookup(conversations, conv);
			znc_conversation_destroy(zncconv);
			g_hash_table_remove(conversations, conv);
			
			inuse = TRUE;
			purple_conv_chat_write(PURPLE_CONV_CHAT(conv), who, _("Playback Complete."), flags|PURPLE_MESSAGE_SYSTEM, time(NULL));
			inuse = FALSE;
			
			cancel = TRUE;
		}
	} else if((purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT && (zncconv = g_hash_table_lookup(conversations, conv)) != NULL) || purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM)  {
		pos = g_strrstr(*message, "[");
		
		if(pos != NULL) {
			stamp = get_time(pos);
			if(stamp != 0) {
				*pos = '\0';
				
				inuse = TRUE;
				
				purple_signal_connect(pidgin_conversations_get_handle(), "conversation-timestamp", plugin, PURPLE_CALLBACK(conversation_timestamp_cb), NULL);
				if(purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT) {
					g_hash_table_insert(zncconv->users, g_strdup(who), "");
					purple_conv_chat_write(PURPLE_CONV_CHAT(conv), who, *message, flags, stamp);
				} else if(purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
					purple_conv_im_write(PURPLE_CONV_IM(conv), who, *message, flags, stamp);
				}
				purple_signal_disconnect(pidgin_conversations_get_handle(), "conversation-timestamp", plugin, PURPLE_CALLBACK(conversation_timestamp_cb));
				
				inuse = FALSE;
				
				cancel = TRUE;
			}
		} else {
			purple_debug_error(PLUGIN_STATIC_NAME, _("Timestamp could not be interpreted. Please use \"[%%Y-%%m-%%d %%H:%%M:%%S]\" as timestamp format and append it to the message.\n"));
		}
	}
	
	return cancel;
}

void message_parser_init(PurplePlugin *_plugin) {
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
	
	purple_signal_connect(purple_conversations_get_handle(), "writing-chat-msg", plugin, PURPLE_CALLBACK(writing_msg_cb), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "writing-im-msg", plugin, PURPLE_CALLBACK(writing_msg_cb), NULL);
}
