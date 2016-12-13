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


#ifndef __GCR_OBJECT_CHOOSER_H__
#define __GCR_OBJECT_CHOOSER_H__

#include <gtk/gtk.h>

typedef struct _GcrObjectChooser GcrObjectChooser;

struct _GcrObjectChooserInterface
{
	GTypeInterface g_iface;

	void (*set_uri) (GcrObjectChooser *self, const gchar *uri);
	gchar *(*get_uri) (GcrObjectChooser *self);
};

#define GCR_TYPE_OBJECT_CHOOSER gcr_object_chooser_get_type ()
G_DECLARE_INTERFACE (GcrObjectChooser, gcr_object_chooser, GCR, OBJECT_CHOOSER, GObject)

void gcr_object_chooser_set_uri (GcrObjectChooser *self, const gchar *uri);

gchar *gcr_object_chooser_get_uri (GcrObjectChooser *self);

#endif /* __GCR_OBJECT_CHOOSER_H__ */
