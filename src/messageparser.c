/* Copyright (C) 2009-2018 Konrad Gräfe <konradgraefe@aol.com>
 *
 * This software may be modified and distributed under the terms
 * of the GPLv2 license. See the COPYING file for details.
 */

#include "config.h"
#include "internal.h"

#include "messageparser.h"

#include <conversation.h>
#include <debug.h>
#include <accountopt.h>
#include <gtkconv.h>
#include <signals.h>

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

	return mktime(&t);
}

static void (*pidgin_write_im)(
	PurpleConversation *conv, const char *who, const char *message,
	PurpleMessageFlags flags, time_t mtime
);
static void write_im(
	PurpleConversation *conv, const char *who, const char *message,
	PurpleMessageFlags flags, time_t mtime
) {
	PurpleAccount *acc;
	gchar *msg, *swap;
	time_t stamp;

	acc = purple_conversation_get_account(conv);
	if(!acc) {
		pidgin_write_im(conv, who, message, flags, mtime);
		return;
	}
	if(!purple_account_get_bool(acc, "uses_znc_bouncer", FALSE)) {
		pidgin_write_im(conv, who, message, flags, mtime);
		return;
	}

	msg = g_strdup(message);

	swap = g_strrstr(msg, "***SWAP***");
	if(swap && swap[10] == '\0') {
		*swap = '\0';
		flags &= ~PURPLE_MESSAGE_RECV;
		flags |= PURPLE_MESSAGE_SEND;
	}

	stamp = get_time(&msg, purple_account_get_int(acc, "znc_time_offset", 0));
	if(!stamp) {
		stamp = mtime;
	}

	pidgin_write_im(conv, who, msg, flags, stamp);

	g_free(msg);
}

static void (*pidgin_write_chat)(
	PurpleConversation *conv, const char *who, const char *message,
	PurpleMessageFlags flags, time_t mtime
);
static void write_chat(
	PurpleConversation *conv, const char *who, const char *message,
	PurpleMessageFlags flags, time_t mtime
) {
	PurpleConvChat *chat;
	PurpleAccount *acc;
	gchar *msg;
	time_t stamp;
	GList *parted, *ignored, *l;

	acc = purple_conversation_get_account(conv);
	if(!acc) {
		pidgin_write_chat(conv, who, message, flags, mtime);
		return;
	}
	chat = PURPLE_CONV_CHAT(conv);
	if(!chat) {
		pidgin_write_chat(conv, who, message, flags, mtime);
		return;
	}
	if(!purple_account_get_bool(acc, "uses_znc_bouncer", FALSE)) {
		pidgin_write_chat(conv, who, message, flags, mtime);
		return;
	}

	if(purple_utf8_strcasecmp(who, "***") == 0) {
		if(
			purple_utf8_strcasecmp(message, "Buffer Playback...") == 0 ||
			purple_utf8_strcasecmp(message, "Starting Buffer Playback...") == 0
		) {
			pidgin_write_chat(
				conv, who, _("Buffer Playback..."),
				flags | PURPLE_MESSAGE_SYSTEM, time(NULL)
			);

			purple_conversation_set_data(conv, "znc-playback", (gpointer)TRUE);

			return;
		}

		if(
			purple_utf8_strcasecmp(message, "Playback Complete.") == 0 ||
			purple_utf8_strcasecmp(message, "Buffer Playback Complete.") == 0
		) {
			pidgin_write_chat(
				conv, who, _("Playback Complete."),
				flags | PURPLE_MESSAGE_SYSTEM, time(NULL)
			);

			purple_conversation_set_data(conv, "znc-playback", FALSE);

			/* Remove all users from the chat that are in the playback but
			 * not present anymore. This gives the UI the chance to mark
			 * them as parted (i.e. Pidgin displays them in italic).
			 *
			 * In order to avoid a lot "user left the room" messages, we
			 * set those users on the ignore list during that operation.
			 */
			parted = purple_conversation_get_data(conv, "znc-parted");
			ignored = purple_conv_chat_get_ignored(chat);
			purple_conv_chat_set_ignored(chat, parted);
			purple_conv_chat_remove_users(chat, parted, NULL);
			purple_conv_chat_set_ignored(chat, ignored);

			purple_conversation_set_data(conv, "znc-parted", NULL);
			for(l = parted; l != NULL; l = l->next) {
				g_free(l->data);
			}
			g_list_free(parted);

			return;
		}

		pidgin_write_chat(conv, who, message, flags, mtime);
		return;
	}

	if((gboolean)purple_conversation_get_data(conv, "znc-playback") == FALSE) {
		pidgin_write_chat(conv, who, message, flags, mtime);
		return;
	}

	msg = g_strdup(message);

	stamp = get_time(&msg, purple_account_get_int(acc, "znc_time_offset", 0));
	if(!stamp) {
		stamp = mtime;
	}

	/* Add the nick to the list of parted users, if it is not
	 * present in the chat anymore.
	 */
	if(purple_conv_chat_cb_find(chat, who) == NULL) {
		parted = purple_conversation_get_data(conv, "znc-parted");
		for(l = parted; l != NULL; l = l->next) {
			if(purple_utf8_strcasecmp(l->data, who) == 0) {
				break;
			}
		}
		if(l == NULL) {
			parted = g_list_append(parted, g_strdup(who));
			purple_conversation_set_data(conv, "znc-parted", parted);
		}
	}

	pidgin_write_chat(conv, who, msg, flags, stamp);

	g_free(msg);
}

void message_parser_init(void) {
	PurpleConversationUiOps *ops = pidgin_conversations_get_conv_ui_ops();

	if(ops->write_im != NULL) {
		pidgin_write_im = ops->write_im;
	} else {
		pidgin_write_im = purple_conversation_write;
	}
	ops->write_im = write_im;

	if(ops->write_chat != NULL) {
		pidgin_write_chat = ops->write_chat;
	} else {
		pidgin_write_chat = purple_conversation_write;
	}
	ops->write_chat = write_chat;
}
void message_parser_uninit(void) {
	PurpleConversationUiOps *ops = pidgin_conversations_get_conv_ui_ops();

	if(pidgin_write_im != purple_conversation_write) {
		ops->write_im = pidgin_write_im;
	} else {
		ops->write_im = NULL;
	}
		
	if(pidgin_write_chat != purple_conversation_write) {
		ops->write_chat = pidgin_write_chat;
	} else {
		ops->write_chat = NULL;
	}
}
