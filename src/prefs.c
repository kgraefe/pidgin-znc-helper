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

#include <gtk/gtk.h>

#include <account.h>
#include <gtkutils.h>
#include <debug.h>

#include "prefs.h"

#if (GTK_MAJOR_VERSION <= 2 && GTK_MINOR_VERSION < 8)
	#define GTK_STOCK_INFO GTK_STOCK_DIALOG_INFO
#endif

enum {
	ACCOUNT_LIST_COL_USES_ZNC,
	ACCOUNT_LIST_COL_PROTOCOL_ICON,
	ACCOUNT_LIST_COL_ACCOUNT_USERNAME,
	ACCOUNT_LIST_COL_TIME_OFFSET,
	ACCOUNT_LIST_COL_TIME_UNIT,
	ACCOUNT_LIST_COL_ACCOUNT,
	ACCOUNT_LIST_NUM_COLUMNS
};

static GtkWidget *make_info_widget(
	const gchar *markup, gchar *stock_id, gboolean indent
) {
	GtkWidget *infobox, *label, *img, *align;

	if(!markup) return NULL;

	infobox = gtk_hbox_new(FALSE, 5);

	if(indent) {
		label = gtk_label_new("");
		gtk_box_pack_start(GTK_BOX(infobox), label, FALSE, FALSE, 10);
	}

	if(stock_id) {
		/* align img to the top of the space */
		align = gtk_alignment_new(0.5, 0, 0, 0);
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

typedef struct _account_list {
	GtkWidget *widget;
	GtkWidget *treeview;

	GtkListStore *model;
} AccountList;

static AccountList account_list = {
	.widget = NULL,
	.treeview = NULL,
	.model = NULL,
};

static void account_list_widget_destroyed_cb() {
	account_list.widget = NULL;
	account_list.treeview = NULL;
	g_object_unref(G_OBJECT(account_list.model));
}

static void znc_toggled_cb(
	GtkCellRendererToggle *renderer, gchar *path_str, gpointer data
) {
	PurpleAccount *account;
	GtkTreeModel *model = GTK_TREE_MODEL(account_list.model);
	GtkTreeIter iter;
	gboolean uses_znc;

	gtk_tree_model_get_iter_from_string(model, &iter, path_str);
	gtk_tree_model_get(model, &iter,
		ACCOUNT_LIST_COL_ACCOUNT, &account,
		ACCOUNT_LIST_COL_USES_ZNC, &uses_znc,
		-1);
	
	purple_account_set_bool(account, "uses_znc_bouncer", !uses_znc);
	gtk_list_store_set(account_list.model, &iter,
		ACCOUNT_LIST_COL_USES_ZNC, !uses_znc,
		-1);
}

static void time_offset_edited_cb(GtkCellRendererToggle *renderer,
	gchar *path_str, gchar *new_text, gpointer data
) {
	PurpleAccount *account;
	GtkTreeModel *model = GTK_TREE_MODEL(account_list.model);
	GtkTreeIter iter;
	gint offset;

	gtk_tree_model_get_iter_from_string(model, &iter, path_str);
	gtk_tree_model_get(model, &iter,
		ACCOUNT_LIST_COL_ACCOUNT, &account,
		-1);

	if(sscanf(new_text, "%i", &offset)==1) {
		purple_account_set_int(account, "znc_time_offset", offset);
		gtk_list_store_set(account_list.model, &iter,
			ACCOUNT_LIST_COL_TIME_OFFSET, offset,
			ACCOUNT_LIST_COL_TIME_UNIT,
			ngettext("hour", "hours", (offset >= 0 ? offset : -offset)),
			-1
		);
	}
}

static GtkWidget *get_account_list_widget() {
	GtkWidget *scrolled_window, *treeview;
	GtkListStore *model;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GValue adjustment;
	GValue editable;
	GValue editing;

	GList *l;
	PurpleAccount *account;
	const char *protocol_id;
	int offset;

	GdkPixbuf *pixbuf;

	adjustment.g_type = 0;
	editable.g_type = 0;
	editing.g_type = 0;

	if(account_list.widget) {
		/* I know this is ugly, I will search for the right destroy-event
		 * later.
		 */
		account_list_widget_destroyed_cb();
	}

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scrolled_window, -1, 200);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC
	);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_SHADOW_ETCHED_IN
	);

	g_signal_connect(
		G_OBJECT(scrolled_window), "destroy_event",
		G_CALLBACK(account_list_widget_destroyed_cb), NULL
	);


	model = gtk_list_store_new(ACCOUNT_LIST_NUM_COLUMNS,
				G_TYPE_BOOLEAN,		/* ACCOUNT_LIST_COL_USES_ZNC */
				GDK_TYPE_PIXBUF,	/* ACCOUNT_LIST_COL_PROTOCOL_ICON */
				G_TYPE_STRING,		/* ACCOUNT_LIST_COL_ACCOUNT_USERNAME */
				G_TYPE_INT,		/* ACCOUNT_LIST_COL_TIME_OFFSET */
				G_TYPE_STRING,		/* ACCOUNT_LIST_COL_TIME_UNIT */
				G_TYPE_POINTER		/* ACCOUNT_LIST_COL_ACCOUNT */
				);
	account_list.model = model;

	/* Liste füllen... */
	for(l = purple_accounts_get_all(); l != NULL; l = l->next) {
		account = (PurpleAccount *)l->data;
		protocol_id = purple_account_get_protocol_id(account);

		if(protocol_id && purple_utf8_strcasecmp(protocol_id, "prpl-irc")==0) {
			pixbuf = pidgin_create_prpl_icon(account, PIDGIN_PRPL_ICON_MEDIUM);
			if ((pixbuf != NULL) && purple_account_is_disconnected(account))
				gdk_pixbuf_saturate_and_pixelate(pixbuf, pixbuf, 0.0, FALSE);
			offset = purple_account_get_int(account, "znc_time_offset", 0);

			gtk_list_store_append(model, &iter);
			gtk_list_store_set(model, &iter,
				ACCOUNT_LIST_COL_USES_ZNC, purple_account_get_bool(account, "uses_znc_bouncer", FALSE),
				ACCOUNT_LIST_COL_PROTOCOL_ICON, pixbuf,
				ACCOUNT_LIST_COL_ACCOUNT_USERNAME, purple_account_get_username(account),
				ACCOUNT_LIST_COL_TIME_OFFSET, offset,
				ACCOUNT_LIST_COL_TIME_UNIT, ngettext("hour", "hours", (offset >= 0 ? offset : -offset)),
				ACCOUNT_LIST_COL_ACCOUNT, account,
				-1
			);

			if(pixbuf != NULL) {
				g_object_unref(G_OBJECT(pixbuf));
			}
		}
	}

	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	account_list.treeview = treeview;

	/* ZNC-Spalte */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("ZNC"));
	gtk_tree_view_column_set_clickable(column, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(
		column, renderer, "active", ACCOUNT_LIST_COL_USES_ZNC
	);

	g_signal_connect(
		G_OBJECT(renderer), "toggled",
		G_CALLBACK(znc_toggled_cb), NULL
	);

	/* Account-Spalte */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Account"));
	gtk_tree_view_column_set_clickable(column, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(
		column, renderer, "pixbuf", ACCOUNT_LIST_COL_PROTOCOL_ICON
	);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(
		column, renderer, "text", ACCOUNT_LIST_COL_ACCOUNT_USERNAME
	);

	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(treeview));
	gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);

	/* Offset-Spalte */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Time offset"));
	gtk_tree_view_column_set_clickable(column, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	g_value_init(&adjustment, G_TYPE_OBJECT);
	g_value_set_object(
		&adjustment, gtk_adjustment_new(1.0, -23.0, 23.0, 1.0, 1.0, 0.0)
	);
	g_value_init(&editable, G_TYPE_BOOLEAN);
	g_value_set_boolean(&editable, TRUE);
	g_value_init(&editing, G_TYPE_BOOLEAN);
	g_value_set_boolean(&editing, TRUE);

	renderer = gtk_cell_renderer_spin_new();
	g_object_set_property(G_OBJECT(renderer), "adjustment", &adjustment);
	g_object_set_property(G_OBJECT(renderer), "editable", &editable);
	g_object_set_property(G_OBJECT(renderer), "sensitive", &editing);
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(
		column, renderer, "text", ACCOUNT_LIST_COL_TIME_OFFSET
	);
	g_signal_connect(
		G_OBJECT(renderer), "edited",
		G_CALLBACK(time_offset_edited_cb), NULL
	);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(
		column, renderer, "text", ACCOUNT_LIST_COL_TIME_UNIT
	);


	account_list.widget = scrolled_window;
	return account_list.widget;
}

GtkWidget *get_pref_frame(PurplePlugin *plugin) {
	GtkWidget *frame, *vbox, *infobox;

	frame = gtk_vbox_new(FALSE, 18);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 12);
	
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	
	infobox = make_info_widget(
		_("Please make sure to set up your ZNC to <b>append</b> the timestamp using the following format:\n<b>[%Y-%m-%d %H:%M:%S]</b>"),
		GTK_STOCK_INFO, FALSE
	);
	gtk_box_pack_start(GTK_BOX(vbox), infobox, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), get_account_list_widget(), TRUE, TRUE, 0);

	return frame;
}

