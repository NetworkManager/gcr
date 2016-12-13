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

#ifndef __GCR_OBJECT_CHOOSER_BUTTON_H__
#define __GCR_OBJECT_CHOOSER_BUTTON_H__

#include <gtk/gtk.h>

#include "gcr-object-chooser.h"

typedef struct _GcrObjectChooserButtonPrivate GcrObjectChooserButtonPrivate;

struct _GcrObjectChooserButton
{
        GtkButton parent_instance;
        GcrObjectChooserButtonPrivate *priv;
};

#define GCR_TYPE_OBJECT_CHOOSER_BUTTON gcr_object_chooser_button_get_type ()
G_DECLARE_FINAL_TYPE (GcrObjectChooserButton, gcr_object_chooser_button, GCR, OBJECT_CHOOSER_BUTTON, GtkButton)

GtkWidget *gcr_object_chooser_button_new (GtkWindow *parent_window);

#endif /* __GCR_OBJECT_CHOOSER_BUTTON_H__ */