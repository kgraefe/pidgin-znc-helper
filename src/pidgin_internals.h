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

#ifndef PIDGIN_INTERNALS_H
#define PIDGIN_INTERNALS_H

#include <gtkconv.h>
#include <gtkimhtml.h>

GtkTextTag *get_buddy_tag(PurpleConversation *conv, const char *who, PurpleMessageFlags flag, gboolean create);
const char *get_text_tag_color(GtkTextTag *tag);
const GdkColor *get_nick_color(PidginConversation *gtkconv, const char *name);

#endif /* PIDGIN_INTERNALS_H */
