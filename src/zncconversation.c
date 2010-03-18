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

#include "zncconversation.h"
#include "pidgin_internals.h"

#include <debug.h>

static void make_italic(gpointer key, gpointer value, gpointer user_data) {
	PurpleConversation *conv;
	GtkTextTag *tag;

	conv = (PurpleConversation *)user_data;

	if ((tag = get_buddy_tag(conv, key, 0, FALSE)))
		g_object_set(G_OBJECT(tag), "style", PANGO_STYLE_ITALIC, NULL);
	if ((tag = get_buddy_tag(conv, key, PURPLE_MESSAGE_NICK, FALSE)))
		g_object_set(G_OBJECT(tag), "style", PANGO_STYLE_ITALIC, NULL);
		
	return;
}

ZNCConversation *znc_conversation_new(PurpleConversation *conv) {
	ZNCConversation * ret;

	ret = g_malloc(sizeof(ZNCConversation));
	ret->prplconv = conv;
	ret->users = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	return ret;
}

void znc_conversation_destroy(ZNCConversation *zncconv) {
	GList *cur;

	PurpleConvChatBuddy *buddy;
	PurpleConversation *conv;
	PidginConversation *gtkconv;
	PurpleConvChat *chat;

	conv = zncconv->prplconv;
	gtkconv = PIDGIN_CONVERSATION(conv);
	chat = PURPLE_CONV_CHAT(conv);


	for(cur = purple_conv_chat_get_users(chat); cur != NULL; cur = cur->next) {
		buddy = (PurpleConvChatBuddy *)cur->data;
		g_hash_table_remove(zncconv->users, buddy->name);
	}
	
	g_hash_table_foreach(zncconv->users, make_italic, conv);

	g_hash_table_destroy(zncconv->users);
	g_free(zncconv);
}

