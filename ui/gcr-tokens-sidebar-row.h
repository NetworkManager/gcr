/*
 * Copyright (C) 2016 Lubomir Rintel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GCR_TOKENS_SIDEBAR_ROW_H__
#define __GCR_TOKENS_SIDEBAR_ROW_H__

#include <gtk/gtk.h>
#include <gck/gck.h>

typedef struct _GcrTokensSidebarRowPrivate GcrTokensSidebarRowPrivate;

struct _GcrTokensSidebarRow
{
	GtkListBoxRow parent_instance;
	GcrTokensSidebarRowPrivate *priv;
};

#define GCR_TYPE_TOKENS_SIDEBAR_ROW gcr_tokens_sidebar_row_get_type ()
G_DECLARE_FINAL_TYPE (GcrTokensSidebarRow, gcr_tokens_sidebar_row, GCR, TOKENS_SIDEBAR_ROW, GtkListBoxRow);

GtkWidget *gcr_tokens_sidebar_row_new (GckSlot *slot);

GckSlot *gcr_tokens_sidebar_row_get_slot (GcrTokensSidebarRow *self);

#endif /* __GCR_TOKENS_SIDEBAR_ROW_H__ */
