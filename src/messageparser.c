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

#include "config.h"
#include "internal.h"

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
static void pidgin_conv_calculate_newday(
	PidginConversation *gtkconv, time_t mtime
) {
	struct tm *tm = localtime(&mtime);

	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
	tm->tm_mday++;

	gtkconv->newday = mktime(tm);
}

static gchar *conversation_timestamp_cb(
	PurpleConversation *conv, time_t mtime, gboolean _show_date
) {
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
	
	/* First message in playback */
	if (gtkconv->newday == (-1)) {
		if(
			(tm_msg.tm_year != tm_now.tm_year) ||
			(tm_msg.tm_mon != tm_now.tm_mon) ||
			(tm_msg.tm_mday != tm_now.tm_mday)
		) {
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

static time_t get_time(gchar **message, int offset) {
	struct tm t;
	gchar *timestamp, *tmp, *tail;
	
	int read = 0;
	int year = 0;
	int month = 0;

	timestamp = g_strrstr(*message, " [");
	if(!timestamp) return 0;

	tail = purple_markup_strip_html(timestamp + 22);
	if(strlen(tail) > 0) {
		g_free(tail);
		return 0;
	}
	g_free(tail);
	
	read = sscanf(timestamp,
		" [%04d-%02d-%02d %02d:%02d:%02d]",
		&year, &month, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec
	);
	t.tm_isdst = (-1);
	
	t.tm_hour += offset;
	
	if(read != 6 || year <= 1900) return 0;
	
	t.tm_year = year - 1900;
	t.tm_mon = month - 1;

	*timestamp = '\0';
	tmp = g_strdup_printf("%s%s", *message, timestamp + 22);
	g_free(*message);
	*message = tmp;

	purple_debug_info(PLUGIN_STATIC_NAME, "%s\n", *message);
	
	return mktime(&t);
}
static gboolean writing_msg_cb(
	PurpleAccount *account, const char *who,
	char **message, PurpleConversation *conv, PurpleMessageFlags flags
) {
	ZNCConversation *zncconv = NULL;
	PidginConversation *gtkconv;
	gboolean cancel = FALSE;
	
	static gboolean inuse = FALSE;
	time_t stamp;
	char *swap = NULL;

	
	if(inuse) return FALSE;
	if(!purple_account_get_bool(account, "uses_znc_bouncer", FALSE)) {
		return FALSE;
	}

	purple_debug_info(PLUGIN_STATIC_NAME, "%s: %s\n", who, *message);
	
	gtkconv = PIDGIN_CONVERSATION(conv);

	if(purple_utf8_strcasecmp(who, "***") == 0) {
		if(
			purple_utf8_strcasecmp(*message, "Buffer Playback...") == 0 ||
			purple_utf8_strcasecmp(*message, "Starting Buffer Playback...") == 0
		) {
			zncconv = znc_conversation_new(conv);
			g_hash_table_insert(conversations, conv, zncconv);
			
			inuse = TRUE;
			purple_conv_chat_write(
				PURPLE_CONV_CHAT(conv), who, _("Buffer Playback..."),
				flags|PURPLE_MESSAGE_SYSTEM, time(NULL)
			);
			
			gtkconv->newday = (-1);
			
			inuse = FALSE;
			
			cancel = TRUE;
		} else if(
			purple_utf8_strcasecmp(*message, "Playback Complete.") == 0 ||
			purple_utf8_strcasecmp(*message, "Buffer Playback Complete.") == 0
		) {
			zncconv = g_hash_table_lookup(conversations, conv);
			znc_conversation_destroy(zncconv);
			g_hash_table_remove(conversations, conv);
			
			inuse = TRUE;
			purple_conv_chat_write(
				PURPLE_CONV_CHAT(conv), who, _("Playback Complete."),
				flags|PURPLE_MESSAGE_SYSTEM, time(NULL)
			);
			inuse = FALSE;
			
			cancel = TRUE;
		}
	} else if(
		(
			purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT &&
			(zncconv = g_hash_table_lookup(conversations, conv)) != NULL
		) || purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM
	)  {
		if((swap = g_strrstr(*message, "***SWAP***")) && swap[10] == '\0') {
			*swap = '\0';
			flags &= ~PURPLE_MESSAGE_RECV;
			flags |= PURPLE_MESSAGE_SEND;
		}
		stamp = get_time(message,
			purple_account_get_int(account, "znc_time_offset", 0)
		);
		if(swap || stamp) {
			inuse = TRUE;

			if(stamp == 0) stamp = time(NULL);
			
			purple_signal_connect_priority(
				pidgin_conversations_get_handle(),
				"conversation-timestamp", plugin,
				PURPLE_CALLBACK(conversation_timestamp_cb), NULL,
				PURPLE_SIGNAL_PRIORITY_LOWEST
			);
			if(purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT) {
				g_hash_table_insert(zncconv->users, g_strdup(who), "");
				purple_conv_chat_write(PURPLE_CONV_CHAT(conv),
					who, *message, flags, stamp
				);
			} else if(
				purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM
			) {
				purple_conv_im_write(PURPLE_CONV_IM(conv),
					who, *message, flags, stamp
				);
			}
			purple_signal_disconnect(
				pidgin_conversations_get_handle(), "conversation-timestamp",
				plugin, PURPLE_CALLBACK(conversation_timestamp_cb)
			);
			
			inuse = FALSE;
			
			cancel = TRUE;
		} else {
			purple_debug_error(PLUGIN_STATIC_NAME,
				_("Timestamp could not be interpreted.\n")
			);
		}
	}
	
	return cancel;
}

void message_parser_init(PurplePlugin *_plugin) {
	plugin = _plugin;
	conversations = g_hash_table_new(NULL, NULL);
	
	purple_signal_connect(
		purple_conversations_get_handle(), "writing-chat-msg",
		plugin, PURPLE_CALLBACK(writing_msg_cb), NULL
	);
	purple_signal_connect(
		purple_conversations_get_handle(), "writing-im-msg",
		plugin, PURPLE_CALLBACK(writing_msg_cb), NULL
	);
}
