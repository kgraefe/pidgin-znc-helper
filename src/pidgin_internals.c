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

#include "pidgin_internals.h"

GtkTextTag *get_buddy_tag(PurpleConversation *conv, const char *who, PurpleMessageFlags flag, gboolean create)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	GtkTextTag *buddytag;
	gchar *str;
	gboolean highlight = (flag & PURPLE_MESSAGE_NICK);
	GtkTextBuffer *buffer = GTK_IMHTML(gtkconv->imhtml)->text_buffer;

	str = g_strdup_printf(highlight ? "HILIT %s" : "BUDDY %s", who);

	buddytag = gtk_text_tag_table_lookup(
			gtk_text_buffer_get_tag_table(buffer), str);

	if (buddytag == NULL && create) {
		if (highlight)
			buddytag = gtk_text_buffer_create_tag(buffer, str,
					"weight", PANGO_WEIGHT_BOLD,
					NULL);
		else
			buddytag = gtk_text_buffer_create_tag(
					buffer, str,
					"weight", purple_find_buddy(purple_conversation_get_account(conv), who) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
					NULL);

		g_object_set_data(G_OBJECT(buddytag), "cursor", "");
		//g_signal_connect(G_OBJECT(buddytag), "event",
		//		G_CALLBACK(buddytag_event), conv);
	}

	g_free(str);

	return buddytag;
}


