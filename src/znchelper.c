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
#include <gtkconv.h>
#include <debug.h>
#include <version.h>

#include "prefs.h"


#define irc_next_field_or_return_false(s)  \
		s = strchr(s, ' '); \
		if(s) {s++;} else {return FALSE;}
#define irc_next_field_or_return(s)  \
		s = strchr(s, ' '); \
		if(s) {s++;} else {return;}

PurplePlugin *prpl_irc;
PurplePluginProtocolInfo *irc_extra_info;

#define ZNC_CONV_STATE_START 0
#define ZNC_CONV_STATE_REPLAY 1
#define ZNC_CONV_STATE_DONE 2

struct znc_conn {
	gboolean server_time_enabled;
	time_t server_time;

	gboolean self_message_enabled;
	gboolean self_message;
};
GHashTable *znc_conns;


static void irc_send_raw(PurpleConnection *gc, const char *str) {
	if(!irc_extra_info->send_raw) {
		error("Could not send raw to IRC!\n");
		return;
	}
	irc_extra_info->send_raw(gc, str, -1);
}
static void irc_sending_text_cb(PurpleConnection *gc, char **text, void *p) {
	struct znc_conn *znc;

	if(purple_connection_get_state(gc) != PURPLE_CONNECTING) {
		return;
	}
	if(!purple_account_get_bool(gc->account, "uses_znc_bouncer", FALSE)) {
		return;
	}

	znc = g_hash_table_lookup(znc_conns, gc);
	if(!znc) {
		znc = g_new0(struct znc_conn, 1);
		g_hash_table_insert(znc_conns, gc, znc);

		irc_send_raw(gc, "CAP REQ :znc.in/server-time-iso\r\n");
		irc_send_raw(gc, "CAP REQ :znc.in/self-message\r\n");
	}
}


static struct znc_conn *conversation_get_znc(PurpleConversation *conv) {
	PurpleConnection *gc;
	PurpleAccount *account;

	account = purple_conversation_get_account(conv);
	if(!account) {
		return NULL;
	}
	gc = purple_account_get_connection(account);
	if(!gc) {
		return NULL;
	}
	return g_hash_table_lookup(znc_conns, gc);
}
static void (*pidgin_write_chat)(
	PurpleConversation *conv, const char *who, const char *message,
	PurpleMessageFlags flags, time_t mtime
);
static void znc_write_chat(
	PurpleConversation *conv, const char *who, const char *message,
	PurpleMessageFlags flags, time_t mtime
) {
	PurpleConvChat *chat;
	struct znc_conn *znc;
	GList *parted, *l;
	gint state;

	znc = conversation_get_znc(conv);
	if(!znc) {
		goto exit;
	}

	if(znc->server_time) {
		mtime = znc->server_time;
		znc->server_time = 0;
	}

	chat = PURPLE_CONV_CHAT(conv);
	if(!chat) {
		goto exit;
	}

	if((flags & PURPLE_MESSAGE_SYSTEM) == 0) {
		state = GPOINTER_TO_INT(purple_conversation_get_data(conv, "znc-state"));
		switch(state) {
		case ZNC_CONV_STATE_START:
			pidgin_write_chat(
				conv, "***", _("Buffer Playback..."),
				PURPLE_MESSAGE_NO_LOG | PURPLE_MESSAGE_SYSTEM, time(NULL)
			);
			purple_conversation_set_data(conv,
				"znc-state", GINT_TO_POINTER(ZNC_CONV_STATE_REPLAY)
			);
			/* fall through */

		case ZNC_CONV_STATE_REPLAY:
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
			break;
		}
	}

exit:
	pidgin_write_chat(conv, who, message, flags, mtime);
}
static void (*pidgin_write_im)(
	PurpleConversation *conv, const char *who,
   const char *message, PurpleMessageFlags flags, time_t mtime
);
static void znc_write_im(
	PurpleConversation *conv, const char *who,
   const char *message, PurpleMessageFlags flags, time_t mtime
) {
	struct znc_conn *znc;

	znc = conversation_get_znc(conv);
	if(!znc) {
		goto exit;
	}

	if(znc->server_time) {
		mtime = znc->server_time;
		znc->server_time = 0;
	}
	if(znc->self_message) {
		flags &= ~PURPLE_MESSAGE_RECV;
		flags |= PURPLE_MESSAGE_SEND;

		znc->self_message = FALSE;
	}

exit:
	pidgin_write_im(conv, who, message, flags, mtime);
}


static void parse_server_time(
	PurpleConnection *gc, struct znc_conn *znc, char **text
) {
	/* Example:
	 *     @time=2018-07-21T09:26:22.353Z :irc.znc.in CAP unknown-nick ACK :znc.in/server-time-iso
	 */

	GTimeVal t;
	char *timestamp, *delimiter;
	int offset;

	if(strncmp(*text, "@time=", 5) != 0) {
		return;
	}

	timestamp = *text + 6;
	delimiter = g_strstr_len(*text, -1, " ");
	if(!delimiter) {
		return;
	}
	*text = delimiter + 1;
	*delimiter = '\0';

	if(!g_time_val_from_iso8601(timestamp, &t)) {
		return;
	}

	offset = purple_account_get_int(gc->account, "znc_time_offset", 0);
	if(offset) {
		/* offset is in hours and g_time_val_add() wants microseconds... */
		g_time_val_add(&t, offset * 60*60*1000*1000);
	}
	znc->server_time = (time_t)t.tv_sec;
}
static void parse_self_message(
	PurpleConnection *gc, struct znc_conn *znc, char **text
) {
	/* Example:
	 *     :ploppy!~kgraefe@p54BCACA3.dip0.t-ipconnect.de PRIVMSG kgraefe :self-message text
	 */

	char *p = *text;
	char *from, *to, *msg, *end, *new;
	const char *username;

	if(*p != ':') {
		return;
	}
	from = p + 1;

	irc_next_field_or_return(p);
	if(strncmp("PRIVMSG ", p, 8) != 0) {
		return;
	}

	username = purple_connection_get_display_name(gc);

	end = strchr(from, '!');
	if(!end) {
		end = p - 1;
	}
	if(
		strlen(username) != (end - from) ||
		strncmp(username, from, (end - from)) != 0
	) {
		return;
	}

	irc_next_field_or_return(p);
	to = p;
	if(*to == '#') {
		return;
	}

	/* This is a self-message in a private chat. We need to swap the sender and
	 * recipient in order to get the message through the IRC plugin. Luckily we
	 * know that the new raw message will be shorter or equal to the original
	 * message so we can just overwrite the buffer.
	 */
	irc_next_field_or_return(p);
	msg = p;

	new = g_strdup_printf(
		":%.*s PRIVMSG %.*s %s"
		, (msg - 1 - to), to
		, (end - from), from
		, msg
	);
	g_strlcpy(*text, new, strlen(*text));
	g_free(new);
	znc->self_message = TRUE;
}
static void parse_endofwho(PurpleConnection *gc, char **text) {
	/* Example:
	 *     :weber.freenode.net 315 ploppy #pidgin-znc-test :End of /WHO list.
	 */

	PurpleAccount *account;
	PurpleConversation *conv;
	PurpleConvChat *chat;
	GList *parted, *ignored, *l;
	char *p = *text;
	char *channel = NULL, *start, *end;
	gint state;

	irc_next_field_or_return(p);
	if(strncmp("315 ", p, 4) != 0) {
		goto exit;
	}

	irc_next_field_or_return(p);
	irc_next_field_or_return(p);

	start = p;
	irc_next_field_or_return(p);
	end = p;

	channel = g_strndup(start, (end - start - 1));

	if(*channel != '#') {
		goto exit;
	}

	account = purple_connection_get_account(gc);
	if(!account) {
		goto exit;
	}

	conv = purple_find_conversation_with_account(
		PURPLE_CONV_TYPE_CHAT, channel, account
	);
	if(!conv) {
		goto exit;
	}

	chat = PURPLE_CONV_CHAT(conv);
	if(!chat) {
		goto exit;
	}

	state = GPOINTER_TO_INT(purple_conversation_get_data(conv, "znc-state"));
	purple_conversation_set_data(conv,
		"znc-state", GINT_TO_POINTER(ZNC_CONV_STATE_DONE)
	);

	if(state == ZNC_CONV_STATE_START) {
		/* We had no replay, so nothing to do. */
		goto exit;
	}

	pidgin_write_chat(
		conv, "***", _("Playback Complete."),
		PURPLE_MESSAGE_NO_LOG | PURPLE_MESSAGE_SYSTEM, time(NULL)
	);

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

exit:
	g_free(channel);
}
static gboolean matches_cap_ack(const char *cap, const char *text) {
	const char *p = text;

	/* Example:
	 *     @time=2018-07-21T09:26:22.353Z :irc.znc.in CAP unknown-nick ACK :znc.in/server-time-iso
	 *
	 * Note that the time stamp may or may not be present depending on the
	 * version of ZNC and the order of CAP REQ's.
	 */
	if(p && strncmp("@time=", p, 5) == 0) {
		irc_next_field_or_return_false(p);
	}
	if(*p != ':') {
		return FALSE;
	}

	irc_next_field_or_return_false(p);
	if(strncmp("CAP ", p, 4) != 0) {
		return FALSE;
	}

	irc_next_field_or_return_false(p);

	irc_next_field_or_return_false(p);
	if(strncmp("ACK ", p, 4) != 0) {
		return FALSE;
	}

	irc_next_field_or_return_false(p);
	if(strcmp(cap, p) != 0) {
		return FALSE;
	}
	return TRUE;
}
static void irc_receiving_text_cb(PurpleConnection *gc, char **text, void *p) {
	struct znc_conn *znc;

	znc = g_hash_table_lookup(znc_conns, gc);
	if(!znc) {
		return;
	}

	if(purple_connection_get_state(gc) == PURPLE_CONNECTING) {
		if(matches_cap_ack(":znc.in/server-time-iso", *text)) {
			znc->server_time_enabled = TRUE;
		}
		if(matches_cap_ack(":znc.in/self-message", *text)) {
			znc->self_message_enabled = TRUE;
		}
	}

	if(znc->server_time_enabled) {
		parse_server_time(gc, znc, text);
	}
	if(znc->self_message_enabled) {
		parse_self_message(gc, znc, text);
	}
	parse_endofwho(gc, text);
}


static gboolean plugin_load(PurplePlugin *plugin) {
	PurpleConversationUiOps *ops;

	prpl_irc = purple_find_prpl("prpl-irc");
	if(!prpl_irc || !prpl_irc->info || !prpl_irc->info->extra_info) {
		error("Could not find IRC protocol!\n");
		return FALSE;
	}
	irc_extra_info = (PurplePluginProtocolInfo *)prpl_irc->info->extra_info;

	znc_conns = g_hash_table_new_full(NULL, NULL, NULL, g_free);

	purple_signal_connect(
		prpl_irc, "irc-receiving-text", plugin,
		PURPLE_CALLBACK(irc_receiving_text_cb), NULL
	);
	purple_signal_connect(
		prpl_irc, "irc-sending-text", plugin,
		PURPLE_CALLBACK(irc_sending_text_cb), NULL
	);

	/* Hook into conversation between Pidgin and libpurple */
	ops = pidgin_conversations_get_conv_ui_ops();

	pidgin_write_chat = ops->write_chat;
	if(!pidgin_write_chat) {
		pidgin_write_chat = purple_conversation_write;
	}
	ops->write_chat = znc_write_chat;

	pidgin_write_im = ops->write_im;
	if(!pidgin_write_im) {
		pidgin_write_im = purple_conversation_write;
	}
	ops->write_im = znc_write_im;

	return TRUE;
}
static gboolean plugin_unload(PurplePlugin *plugin) {
	PurpleConversationUiOps *ops;

	ops = pidgin_conversations_get_conv_ui_ops();
	if(ops) {
		if(pidgin_write_chat == purple_conversation_write) {
			ops->write_chat = NULL;
		} else {
			ops->write_chat = pidgin_write_chat;
		}
		if(pidgin_write_im == purple_conversation_write) {
			ops->write_im = NULL;
		} else {
			ops->write_im = pidgin_write_im;
		}
	}

	purple_signal_disconnect(
		prpl_irc, "irc-sending-text", plugin,
		PURPLE_CALLBACK(irc_sending_text_cb)
	);
	purple_signal_disconnect(
		prpl_irc, "irc-receiving-text", plugin,
		PURPLE_CALLBACK(irc_receiving_text_cb)
	);

	g_hash_table_destroy(znc_conns);

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
	info.description = _("Pidgin ZNC Helper parses IRC bouncer timestamps and displays them as normal timestamps.");
		
	purple_prefs_add_none(PLUGIN_PREFS_PREFIX);
	purple_prefs_add_int(PLUGIN_PREFS_PREFIX "/offset", 0);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)
