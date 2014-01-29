/*
 * ZNC Helper
 * Copyright (C) 2012 Konrad Gräfe
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
#include "queryfix.h"
#include "pidgin_internals.h"

#include <core.h>
#include <account.h>
#include <debug.h>

static int purple_major_version;
static int purple_minor_version;
static int purple_patch_version;

static char *irc_mask_nick(const char *mask)
{
	char *end, *buf;

	end = strchr(mask, '!');
	if (!end)
		buf = g_strdup(mask);
	else
		buf = g_strndup(mask, end - mask);

	return buf;
}


void (*irc_msg_privmsg_ori)(struct irc_conn *irc, const char *name, const char *from, char **args) = NULL;

static void irc_msg_privmsg(struct irc_conn *irc, const char *name, const char *from, char **args) {
	PurpleConnection *gc = purple_account_get_connection(irc->account);
	char *to, *rawmsg, *nick, *msg;

	if (!args || !args[0] || !args[1]) {
		irc_msg_privmsg_ori(irc, name, from, args);
		return;
	}

	to = args[0];
	rawmsg = args[1];
	nick = irc_mask_nick(from);

	purple_debug_info(PLUGIN_STATIC_NAME, "irc_msg_privmsg: received message %s from %s to %s\n", rawmsg, nick, to);


	if(purple_utf8_strcasecmp(nick, purple_connection_get_display_name(gc))) {
		irc_msg_privmsg_ori(irc, name, from, args);
		g_free(nick);
		return;
	}

	/* Let's asume all channels to start with a # */
	if(*to == '#') {
		irc_msg_privmsg_ori(irc, name, from, args);
		g_free(nick);
		return;
	}

	msg = g_strdup_printf("%s***SWAP***", rawmsg);

	args[0] = nick;
	args[1] = msg;
	irc_msg_privmsg_ori(irc, name, to, args);
	args[0] = to;
	args[1] = rawmsg;

	g_free(msg);
	g_free(nick);
}

static void connection_signed_on_cb(PurpleConnection *gc) {
	PurpleAccount *account;
	struct irc_conn *irc;
	GHashTable *msgs;
	struct irc_msg_old *privmsg_old;
	struct irc_msg_2_10_8 *privmsg_2_10_8;

	if(!gc) return;

	account = gc->account;
	if(!account) return;
	if(!purple_account_get_bool(account, "uses_znc_bouncer", FALSE)) return;

	irc = gc->proto_data;
	if(!irc) return;

	msgs = irc->msgs;
	if(!msgs) return;

    if(purple_major_version > 2 || purple_minor_version > 10 || purple_patch_version > 7) {
        privmsg_2_10_8 = (struct irc_msg_2_10_8 *)g_hash_table_lookup(msgs, "privmsg");
        if(!privmsg_2_10_8) return;

        if(!irc_msg_privmsg_ori) irc_msg_privmsg_ori = privmsg_2_10_8->cb;
        privmsg_2_10_8->cb = irc_msg_privmsg;
    } else {
        privmsg_old = (struct irc_msg_old *)g_hash_table_lookup(msgs, "privmsg");
        if(!privmsg_old) return;

        if(!irc_msg_privmsg_ori) irc_msg_privmsg_ori = privmsg_old->cb;
        privmsg_old->cb = irc_msg_privmsg;
    }
}

void query_fix_init(PurplePlugin *plugin) {
    if(sscanf(
        purple_core_get_version(),
        "%d.%d.%d",
        &purple_major_version,
        &purple_minor_version,
        &purple_patch_version
    ) < 3) {
        purple_debug_warning(PLUGIN_STATIC_NAME, "Could not parse libpurple version. Defaulting to latest kown.\n");
        purple_major_version = 2;
        purple_minor_version = 10;
        purple_patch_version = 8;
    }

	purple_signal_connect(purple_connections_get_handle(), "signed-on", plugin, PURPLE_CALLBACK(connection_signed_on_cb), NULL);
}
