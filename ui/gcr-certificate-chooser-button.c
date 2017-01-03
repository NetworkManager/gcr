/*
 * Copyright (C) 2016,2017 Lubomir Rintel
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

#include "gcr-certificate-chooser-button.h"
#include "gcr-certificate-chooser-dialog.h"

#include "config.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gck/gck.h>

/**
 * SECTION:gcr-certificate-chooser-button
 * @title: GcrCertificateChooserButton
 * @short_description: The PKCS11 Certificate/ Chooser Button
 * @see_also: #GcrCertificateChooser
 *
 * #GcrCertificateChooserButton is the PKCS11 certificate chooser button. It runs the
 * #GcrCertificateChooserDialog when clicked, to choose a certificate.
 */

struct _GcrCertificateChooserButtonPrivate
{
	GcrCertificateChooserDialog *dialog;
	GtkLabel *cert_label;
};

static void gcr_certificate_chooser_iface_init (GcrCertificateChooserInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GcrCertificateChooserButton, gcr_certificate_chooser_button, GTK_TYPE_BUTTON,
                         G_ADD_PRIVATE (GcrCertificateChooserButton)
                         G_IMPLEMENT_INTERFACE (GCR_TYPE_CERTIFICATE_CHOOSER, gcr_certificate_chooser_iface_init));

static char *
uri_to_label (const char *uri)
{
	GckUriData *data;
	gchar *label = NULL;
	gchar *real_uri;
	GError *error = NULL;
	char *scheme;
	char *c;

	if (!uri)
		label = g_strdup (_("(None)"));

	if (!label) {
		scheme = g_uri_parse_scheme (uri);
		if (scheme == NULL) {
			g_warning ("Not an URI: '%s'\n", uri);
		} else if (strcmp (scheme, "pkcs11") == 0) {
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
						label = g_strdup (data->token_info->label);
				}
				gck_uri_data_free (data);
			} else {
				g_warning ("Bad PKCS#11 URI: %s\n", error->message);
				g_error_free (error);
			}
		} else if (strcmp (scheme, "file") == 0) {
			label = g_filename_display_basename (uri);
		} else {
			g_warning ("Unhandled URI scheme: '%s'\n", scheme);
		}
		g_free (scheme);
	}

	if (!label)
		label = g_strdup (_("(Unknown)"));

	return label;
}

static void
update_label (GcrCertificateChooserButton *self)
{
	GcrCertificateChooserButtonPrivate *priv = self->priv;
	const char *uri;
	gchar *label = NULL;

	uri = gcr_certificate_chooser_get_cert_uri (GCR_CERTIFICATE_CHOOSER (priv->dialog));
	label = uri_to_label (uri);
	gtk_label_set_text (priv->cert_label, label);
	g_free (label);
}

static void
set_cert_uri (GcrCertificateChooser *self, const gchar *uri)
{
	GcrCertificateChooserButtonPrivate *priv = GCR_CERTIFICATE_CHOOSER_BUTTON (self)->priv;

	gcr_certificate_chooser_set_cert_uri (GCR_CERTIFICATE_CHOOSER (priv->dialog), uri);
	update_label (GCR_CERTIFICATE_CHOOSER_BUTTON (self));
}

static gchar *
get_cert_password (GcrCertificateChooser *self)
{
	GcrCertificateChooserButtonPrivate *priv = GCR_CERTIFICATE_CHOOSER_BUTTON (self)->priv;

	return gcr_certificate_chooser_get_cert_uri (GCR_CERTIFICATE_CHOOSER (priv->dialog));
}

static void
set_cert_password (GcrCertificateChooser *self, const gchar *password)
{
	GcrCertificateChooserButtonPrivate *priv = GCR_CERTIFICATE_CHOOSER_BUTTON (self)->priv;

	gcr_certificate_chooser_set_cert_password (GCR_CERTIFICATE_CHOOSER (priv->dialog), password);
}

static gchar *
get_cert_uri (GcrCertificateChooser *self)
{
	GcrCertificateChooserButtonPrivate *priv = GCR_CERTIFICATE_CHOOSER_BUTTON (self)->priv;

	return gcr_certificate_chooser_get_cert_uri (GCR_CERTIFICATE_CHOOSER (priv->dialog));
}

static void
set_key_uri (GcrCertificateChooser *self, const gchar *uri)
{
	GcrCertificateChooserButtonPrivate *priv = GCR_CERTIFICATE_CHOOSER_BUTTON (self)->priv;

	gcr_certificate_chooser_set_key_uri (GCR_CERTIFICATE_CHOOSER (priv->dialog), uri);
	update_label (GCR_CERTIFICATE_CHOOSER_BUTTON (self));
}

static gchar *
get_key_uri (GcrCertificateChooser *self)
{
	GcrCertificateChooserButtonPrivate *priv = GCR_CERTIFICATE_CHOOSER_BUTTON (self)->priv;

	return gcr_certificate_chooser_get_key_uri (GCR_CERTIFICATE_CHOOSER (priv->dialog));
}

static void
set_key_password (GcrCertificateChooser *self, const gchar *password)
{
	GcrCertificateChooserButtonPrivate *priv = GCR_CERTIFICATE_CHOOSER_BUTTON (self)->priv;

	gcr_certificate_chooser_set_key_password (GCR_CERTIFICATE_CHOOSER (priv->dialog), password);
}

static gchar *
get_key_password (GcrCertificateChooser *self)
{
	GcrCertificateChooserButtonPrivate *priv = GCR_CERTIFICATE_CHOOSER_BUTTON (self)->priv;

	return gcr_certificate_chooser_get_key_password (GCR_CERTIFICATE_CHOOSER (priv->dialog));
}

static void
gcr_certificate_chooser_iface_init (GcrCertificateChooserInterface *iface)
{
	iface->set_cert_uri = set_cert_uri;
	iface->get_cert_uri = get_cert_uri;
	iface->set_cert_password = set_cert_password;
	iface->get_cert_password = get_cert_password;
	iface->set_key_uri = set_key_uri;
	iface->get_key_uri = get_key_uri;
	iface->set_key_password = set_key_password;
	iface->get_key_password = get_key_password;
}

static void
clicked (GtkButton *button)
{
	GcrCertificateChooserButtonPrivate *priv = GCR_CERTIFICATE_CHOOSER_BUTTON (button)->priv;

	if (gcr_certificate_chooser_dialog_run (priv->dialog))
		update_label (GCR_CERTIFICATE_CHOOSER_BUTTON (button));
}

static void
finalize (GObject *object)
{
	GcrCertificateChooserButtonPrivate *priv = GCR_CERTIFICATE_CHOOSER_BUTTON (object)->priv;

	gtk_widget_destroy (GTK_WIDGET (priv->dialog));
}

static void
gcr_certificate_chooser_button_class_init (GcrCertificateChooserButtonClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = finalize;

        gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gcr/ui/gcr-certificate-chooser-button.ui");
        gtk_widget_class_bind_template_child_private (widget_class, GcrCertificateChooserButton, cert_label);
	gtk_widget_class_bind_template_callback (widget_class, clicked);
}

static void
gcr_certificate_chooser_button_init (GcrCertificateChooserButton *self)
{
	self->priv = gcr_certificate_chooser_button_get_instance_private (self);

	gtk_widget_init_template (GTK_WIDGET (self));
}

/**
 * gcr_certificate_chooser_button_new:
 * @parent_window: (allow-none): parent window of the #GtkObjectChooserDialog or %NULL
 *
 * Creates the new #GcrCertificateChooserButton.
 *
 * Returns: newly created #GcrCertificateChooserButton
 */

GtkWidget *
gcr_certificate_chooser_button_new (GtkWindow *parent_window)
{
	GtkWidget *self;
	GcrCertificateChooserButtonPrivate *priv;

	self = GTK_WIDGET (g_object_new (GCR_TYPE_CERTIFICATE_CHOOSER_BUTTON, NULL));

	priv = GCR_CERTIFICATE_CHOOSER_BUTTON (self)->priv;
	priv->dialog = gcr_certificate_chooser_dialog_new (parent_window);
	update_label (GCR_CERTIFICATE_CHOOSER_BUTTON (self));

	return self;
}
