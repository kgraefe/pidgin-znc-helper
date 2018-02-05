/* Copyright (C) 2009-2018 Konrad Gräfe <konradgraefe@aol.com>
 *
 * This software may be modified and distributed under the terms
 * of the GPLv2 license. See the COPYING file for details.
 */

#include "config.h"
#include "internal.h"
#include "queryfix.h"
#include "pidgin_internals.h"

#include <core.h>
#include <account.h>
#include <debug.h>

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


void (*irc_msg_privmsg_ori)(
	struct irc_conn *irc, const char *name, const char *from, char **args
) = NULL;

static void irc_msg_privmsg(
	struct irc_conn *irc, const char *name, const char *from, char **args
) {
	PurpleConnection *gc = purple_account_get_connection(irc->account);
	char *to, *rawmsg, *nick, *msg;

	if (!args || !args[0] || !args[1]) {
		irc_msg_privmsg_ori(irc, name, from, args);
		return;
	}

	to = args[0];
	rawmsg = args[1];
	nick = irc_mask_nick(from);

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
	struct irc_msg *privmsg;
	struct irc_msg_2_10_8 *privmsg_2_10_8;

	if(!gc) return;

	account = gc->account;
	if(!account) return;
	if(!purple_account_get_bool(account, "uses_znc_bouncer", FALSE)) return;

	irc = gc->proto_data;
	if(!irc) return;

	msgs = irc->msgs;
	if(!msgs) return;

	/* struct irc_msg changed with Pidgin 2.10.8 in a way that makes it
	 * incompatible with our plugin. Unfortunately this was a security fix so
	 * that distributors (lookin' at you, Ubuntu) backported it to older
	 * versions which breaks the former libpurple version check.
	 *
	 * Now we're guessing, yay! If the callback pointer is 2 it is likely that
	 * it is not a function pointer but the number of requested arguments. I
	 * don't know if that works on all platforms...
	 */
	privmsg = (struct irc_msg *)g_hash_table_lookup(msgs, "privmsg");
	if(!privmsg) return;

	if((int)privmsg->cb != 2) {
		/* Seems like we are not on a patched libpurple */
		if(!irc_msg_privmsg_ori) irc_msg_privmsg_ori = privmsg->cb;
		privmsg->cb = irc_msg_privmsg;
	} else {
		/* Is that a patched libpurple? Well, yeah. Probably. */
		privmsg_2_10_8 = (struct irc_msg_2_10_8 *)privmsg;
		if(!irc_msg_privmsg_ori) irc_msg_privmsg_ori = privmsg_2_10_8->cb;
		privmsg_2_10_8->cb = irc_msg_privmsg;
	}
}

void query_fix_init(PurplePlugin *plugin) {
	purple_signal_connect(
		purple_connections_get_handle(), "signed-on",
		plugin, PURPLE_CALLBACK(connection_signed_on_cb), NULL
	);
}
void query_fix_uninit(PurplePlugin *plugin) {
	purple_signal_disconnect(
		purple_connections_get_handle(), "signed-on",
		plugin, PURPLE_CALLBACK(connection_signed_on_cb)
	);
}
