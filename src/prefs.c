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

#include <gtk/gtk.h>

#include "prefs.h"

#if (GTK_MAJOR_VERSION <= 2 && GTK_MINOR_VERSION < 8)
	#define GTK_STOCK_INFO GTK_STOCK_DIALOG_INFO
#endif

static GtkWidget *make_info_widget(gchar *markup, gchar *stock_id, gboolean indent) {
	GtkWidget *infobox, *label, *img, *align;

	if(!markup) return NULL;

	infobox = gtk_hbox_new(FALSE, 5);

	if(indent) {
		label = gtk_label_new("");
		gtk_box_pack_start(GTK_BOX(infobox), label, FALSE, FALSE, 10);
	}

	if(stock_id) {
		align = gtk_alignment_new(0.5, 0, 0, 0); /* align img to the top of the space */
		gtk_box_pack_start(GTK_BOX(infobox), align, FALSE, FALSE, 0);

		img = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU);
		gtk_container_add(GTK_CONTAINER(align), img);
	}

	label = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	gtk_box_pack_start(GTK_BOX(infobox), label, FALSE, FALSE, 0);

	return infobox;
}

GtkWidget *get_pref_frame(PurplePlugin *plugin) {
	GtkWidget *frame, *vbox, *infobox;

	frame = gtk_vbox_new(FALSE, 18);
        gtk_container_set_border_width(GTK_CONTAINER(frame), 12);
	
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	
	infobox = make_info_widget(_("Please make sure to set up your ZNC to <b>append</b> the timestamp using the following format:\n<b>[%Y-%m-%d %H:%M:%S]</b>"), GTK_STOCK_INFO, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), infobox, FALSE, FALSE, 0);

	return frame;
}

