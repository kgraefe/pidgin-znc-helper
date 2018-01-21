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

PurplePlugin *prpl_irc;
PurplePluginProtocolInfo *irc_extra_info;

struct znc_conn {
	gboolean server_time_enabled;
	time_t server_time;
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
	}
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
	}

	if(znc->server_time_enabled) {
		parse_server_time(gc, znc, text);
	}
}

static void (*pidgin_conv_write_conv)(
	PurpleConversation *conv, const char *name, const char *alias,
	const char *message, PurpleMessageFlags flags, time_t mtime
);
static void znc_write_conv(
	PurpleConversation *conv, const char *name, const char *alias,
	const char *message, PurpleMessageFlags flags, time_t mtime
) {
	PurpleConnection *gc;
	PurpleAccount *account;
	struct znc_conn *znc;

	account = purple_conversation_get_account(conv);
	if(!account) {
		goto exit;
	}
	gc = purple_account_get_connection(account);
	if(!gc) {
		goto exit;
	}

	znc = g_hash_table_lookup(znc_conns, gc);
	if(!znc) {
		goto exit;
	}
	if(znc->server_time) {
		mtime = znc->server_time;
		znc->server_time = 0;
	}

exit:
	pidgin_conv_write_conv(conv, name, alias, message, flags, mtime);
}


static gboolean plugin_load(PurplePlugin *plugin) {
	PurpleConversationUiOps *conversation_ui_ops;

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

	/* Let's hook into conversation between Pidgin and libpurple */
	conversation_ui_ops = pidgin_conversations_get_conv_ui_ops();
	pidgin_conv_write_conv = conversation_ui_ops->write_conv;
	conversation_ui_ops->write_conv = znc_write_conv;
	
	return TRUE;
}
static gboolean plugin_unload(PurplePlugin *plugin) {
	PurpleConversationUiOps *conversation_ui_ops;

	conversation_ui_ops = pidgin_conversations_get_conv_ui_ops();
	if(conversation_ui_ops) {
		conversation_ui_ops->write_conv = pidgin_conv_write_conv;;
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
	info.description = _(
        "Pidgin ZNC Helper parses IRC bouncer timestamps and displays them as normal timestamps.\n\n"
        "Therefore, the ZNC needs timestamp needs append the timestamp in the the following format:\n"
		"[%Y-%m-%d %H:%M:%S]\n"
    );
		
	purple_prefs_add_none(PLUGIN_PREFS_PREFIX);
	purple_prefs_add_int(PLUGIN_PREFS_PREFIX "/offset", 0);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)
