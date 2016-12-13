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

#include "gcr-object-chooser.h"
#include "gcr-tokens-sidebar.h"

#include <gtk/gtk.h>

/**
 * SECTION:gcr-object-chooser
 * @title: GcrObjectChooser
 * @short_description: The PKCS11 Object Chooser Interface
 * @see_also: #GcrObjectChooserDialog
 *
 * #GcrObjectChooser is an interface for PKCS11 object chooser widgets.
 */

G_DEFINE_INTERFACE (GcrObjectChooser, gcr_object_chooser, G_TYPE_OBJECT)

static void
gcr_object_chooser_default_init (GcrObjectChooserInterface *iface)
{

	/**
	 * GcrObjectChooser::object-activated
	 *
	 * Emitted when an object choice is confirmed (e.g. with a double click).
	 */
	g_signal_new ("object-activated",
	              G_TYPE_FROM_INTERFACE (iface),
	              G_SIGNAL_RUN_LAST,
	              0, NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE, 0);

	/**
	 * GcrObjectChooser::object-selected
	 *
	 * Emitted when the object is selected; not yet confirmed.
	 */
	g_signal_new ("object-selected",
	              G_TYPE_FROM_INTERFACE (iface),
	              G_SIGNAL_RUN_LAST,
	              0, NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE, 0);
}

/**
 * gcr_object_chooser_set_uri:
 * @self: the #GcrObjectChooser
 * @uri: the object URI
 *
 * Set the URI of the focused object. If it's not yet added, then it will
 * be focused as soon as it appears.
 */
void
gcr_object_chooser_set_uri (GcrObjectChooser *self, const gchar *uri)
{
	g_return_if_fail (GCR_IS_OBJECT_CHOOSER (self));

	GCR_OBJECT_CHOOSER_GET_IFACE (self)->set_uri (self, uri);
}

/**
 * gcr_object_chooser_get_uri:
 * @self: the #GcrObjectChooser
 *
 * Get the URI of the currently selected object.
 *
 * Returns: (transfer full): the URI of the object.
 */
gchar *
gcr_object_chooser_get_uri (GcrObjectChooser *self)
{
	g_return_val_if_fail (GCR_IS_OBJECT_CHOOSER (self), NULL);

	return GCR_OBJECT_CHOOSER_GET_IFACE (self)->get_uri (self);
}
