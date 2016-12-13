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

#include "gcr-object-chooser-button.h"
#include "gcr-object-chooser-dialog.h"

#include "config.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gck/gck.h>

/**
 * SECTION:gcr-object-chooser-button
 * @title: GcrObjectChooserButton
 * @short_description: The PKCS11 Object Chooser Button
 * @see_also: #GcrObjectChooser
 *
 * #GcrObjectChooserButton is the PKCS11 object chooser button. It runs the
 * #GcrObjectChooserDialog when clicked, to choose an object.
 */

struct _GcrObjectChooserButtonPrivate
{
	GtkWidget *dialog;
	GtkLabel *label;

	gchar *uri;
};

static void gcr_object_chooser_iface_init (GcrObjectChooserInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GcrObjectChooserButton, gcr_object_chooser_button, GTK_TYPE_BUTTON,
                         G_ADD_PRIVATE (GcrObjectChooserButton)
                         G_IMPLEMENT_INTERFACE (GCR_TYPE_OBJECT_CHOOSER, gcr_object_chooser_iface_init));

static void
set_uri (GcrObjectChooser *self, const gchar *uri)
{
	GcrObjectChooserButtonPrivate *priv = GCR_OBJECT_CHOOSER_BUTTON (self)->priv;
	GckUriData *data;
	GError *error = NULL;
	gchar *label = NULL;
	gchar *real_uri;
	char *c;

	if (priv->uri) {
		g_free (priv->uri);
		priv->uri = NULL;
	}

	if (uri) {
		priv->uri = g_strdup (uri);
	} else {
		gtk_button_set_label (GTK_BUTTON (self), _("(None)"));
		return;
	}

	/* XXX: Fix GckUri to handle URI attributes instead of chopping them off. */
	real_uri = g_strdup (uri);
	c = strchr (real_uri, '&');
	if (c)
		*c = '\0';
	data = gck_uri_parse (real_uri, GCK_URI_FOR_ANY, &error);
	g_free (real_uri);

	if (data) {
		if (!gck_attributes_find_string (data->attributes, CKA_LABEL, &label)) {
			if (data->token_info)
				label = g_strdup_printf (_("Certificate in %s"), data->token_info->label);
		}
	} else {
		g_warning ("Bad URI: %s\n", error->message);
		g_error_free (error);
	}

	if (!label)
		label = g_strdup (_("(Unknown)"));
	gtk_label_set_text (GTK_LABEL (priv->label), label);
	g_free (label);
	gck_uri_data_free (data);
}

static gchar *
get_uri (GcrObjectChooser *self)
{
	GcrObjectChooserButtonPrivate *priv = GCR_OBJECT_CHOOSER_BUTTON (self)->priv;

	return priv->uri;
}

static void
gcr_object_chooser_iface_init (GcrObjectChooserInterface *iface)
{
	iface->set_uri = set_uri;
	iface->get_uri = get_uri;
}

static void
clicked (GtkButton *button)
{
	GcrObjectChooserButtonPrivate *priv = GCR_OBJECT_CHOOSER_BUTTON (button)->priv;

	if (gtk_dialog_run (GTK_DIALOG (priv->dialog)) == GTK_RESPONSE_ACCEPT) {
		gcr_object_chooser_set_uri (GCR_OBJECT_CHOOSER (button),
		                            gcr_object_chooser_get_uri (GCR_OBJECT_CHOOSER (priv->dialog)));
	}
	gtk_widget_hide (priv->dialog);

}

static void
finalize (GObject *object)
{
	GcrObjectChooserButtonPrivate *priv = GCR_OBJECT_CHOOSER_BUTTON (object)->priv;

	if (priv->uri)
		g_free (priv->uri);
	gtk_widget_destroy (priv->dialog);
}

static void
gcr_object_chooser_button_class_init (GcrObjectChooserButtonClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = finalize;

        gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gcr/ui/gcr-object-chooser-button.ui");
        gtk_widget_class_bind_template_child_private (widget_class, GcrObjectChooserButton, label);
	gtk_widget_class_bind_template_callback (widget_class, clicked);
}

static void
gcr_object_chooser_button_init (GcrObjectChooserButton *self)
{
	self->priv = gcr_object_chooser_button_get_instance_private (self);

	gtk_widget_init_template (GTK_WIDGET (self));
}

/**
 * gcr_object_chooser_button_new:
 * @parent_window: (allow-none): parent window of the #GtkObjectChooserDialog or %NULL
 *
 * Creates the new #GcrObjectChooserButton.
 *
 * Returns: newly created #GcrObjectChooserButton
 */

GtkWidget *
gcr_object_chooser_button_new (GtkWindow *parent_window)
{
	GtkWidget *self;
	GcrObjectChooserButtonPrivate *priv;

	self = GTK_WIDGET (g_object_new (GCR_TYPE_OBJECT_CHOOSER_BUTTON, NULL));

	priv = GCR_OBJECT_CHOOSER_BUTTON (self)->priv;
	priv->dialog = gcr_object_chooser_dialog_new (_("Choose a certificate"),
	                                              parent_window,
	                                              GTK_DIALOG_USE_HEADER_BAR,
	                                              _("_Cancel"), GTK_RESPONSE_CANCEL,
	                                              _("_Select"), GTK_RESPONSE_ACCEPT,
	                                              NULL);

	return self;
}
