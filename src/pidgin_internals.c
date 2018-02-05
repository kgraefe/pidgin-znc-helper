/* Copyright (C) 2009-2018 Konrad Gräfe <konradgraefe@aol.com>
 *
 * This software may be modified and distributed under the terms
 * of the GPLv2 license. See the COPYING file for details.
 */

#include "config.h"
#include "internal.h"

#include "pidgin_internals.h"

GtkTextTag *get_buddy_tag(
	PurpleConversation *conv, const char *who,
	PurpleMessageFlags flag, gboolean create
) {
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	GtkTextTag *buddytag;
	gchar *str;
	PangoWeight weight;
	gboolean highlight = (flag & PURPLE_MESSAGE_NICK);
	GtkTextBuffer *buffer = GTK_IMHTML(gtkconv->imhtml)->text_buffer;

	str = g_strdup_printf(highlight ? "HILIT %s" : "BUDDY %s", who);

	buddytag = gtk_text_tag_table_lookup(
			gtk_text_buffer_get_tag_table(buffer), str);

	if (buddytag == NULL && create) {
		if(highlight) {
			buddytag = gtk_text_buffer_create_tag(buffer, str,
					"weight", PANGO_WEIGHT_BOLD,
					NULL);
		} else {
			if(purple_find_buddy(purple_conversation_get_account(conv), who)) {
				weight = PANGO_WEIGHT_BOLD;
			} else {
				weight = PANGO_WEIGHT_NORMAL;
			}

			buddytag = gtk_text_buffer_create_tag(
				buffer, str, "weight", weight, NULL
			);
		}

		g_object_set_data(G_OBJECT(buddytag), "cursor", "");
	}

	g_free(str);

	return buddytag;
}


