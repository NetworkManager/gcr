/*
 * gnome-keyring
 *
 * Copyright (C) 2016 Red Hat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Lubomir Rintel <lkundrak@v3.sk>
 */

#include "gcr-certificate-chooser.h"

/**
 * SECTION:gcr-certificate-chooser
 * @title: GcrCertificateChooser
 * @short_description: The certificate chooser interface
 * @see_also: #GcrCertificateChooserDialog
 *
 * #GcrCertificateChooser is an interface for selecting certificates and
 * keys from files and PKCS11 tokens.
 */

G_DEFINE_INTERFACE (GcrCertificateChooser, gcr_certificate_chooser, G_TYPE_OBJECT)

static void
gcr_certificate_chooser_default_init (GcrCertificateChooserInterface *iface)
{
	/**
	 * GcrCertificateChooser::certificate-selected
	 *
	 * Emitted when the certificate is selected; not yet confirmed.
	 */
	g_signal_new ("certificate-selected",
	              G_TYPE_FROM_INTERFACE (iface),
	              G_SIGNAL_RUN_LAST,
	              0, NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE, 0);
}

/**
 * gcr_certificate_chooser_set_cert_uri:
 * @self: the #GcrCertificateChooser
 * @cert_uri:
 */
void
gcr_certificate_chooser_set_cert_uri (GcrCertificateChooser *self, const gchar *cert_uri)
{
	g_return_if_fail (GCR_IS_CERTIFICATE_CHOOSER (self));

	GCR_CERTIFICATE_CHOOSER_GET_IFACE (self)->set_cert_uri (self, cert_uri);
}

/**
 * gcr_certificate_chooser_get_cert_uri:
 * @self: the #GcrCertificateChooser
 *
 * Returns: (transfer full):
 */
gchar *
gcr_certificate_chooser_get_cert_uri (GcrCertificateChooser *self)
{
	g_return_val_if_fail (GCR_IS_CERTIFICATE_CHOOSER (self), NULL);

	return GCR_CERTIFICATE_CHOOSER_GET_IFACE (self)->get_cert_uri (self);
}

/**
 * gcr_certificate_chooser_set_cert_password:
 * @self: the #GcrCertificateChooser
 * @cert_password:
 */
void
gcr_certificate_chooser_set_cert_password (GcrCertificateChooser *self, const gchar *cert_password)
{
	g_return_if_fail (GCR_IS_CERTIFICATE_CHOOSER (self));

	GCR_CERTIFICATE_CHOOSER_GET_IFACE (self)->set_cert_password (self, cert_password);
}

/**
 * gcr_certificate_chooser_get_cert_password:
 * @self: the #GcrCertificateChooser
 *
 * Returns: (transfer full):
 */
gchar *
gcr_certificate_chooser_get_cert_password (GcrCertificateChooser *self)
{
	g_return_val_if_fail (GCR_IS_CERTIFICATE_CHOOSER (self), NULL);

	return GCR_CERTIFICATE_CHOOSER_GET_IFACE (self)->get_cert_password (self);
}

/**
 * gcr_certificate_chooser_set_key_uri:
 * @self: the #GcrCertificateChooser
 * @key_uri:
 */
void
gcr_certificate_chooser_set_key_uri (GcrCertificateChooser *self, const gchar *key_uri)
{
	g_return_if_fail (GCR_IS_CERTIFICATE_CHOOSER (self));

	GCR_CERTIFICATE_CHOOSER_GET_IFACE (self)->set_key_uri (self, key_uri);
}

/**
 * gcr_certificate_chooser_get_key_uri:
 * @self: the #GcrCertificateChooser
 *
 * Returns: (transfer full):
 */
gchar *
gcr_certificate_chooser_get_key_uri (GcrCertificateChooser *self)
{
	g_return_val_if_fail (GCR_IS_CERTIFICATE_CHOOSER (self), NULL);

	return GCR_CERTIFICATE_CHOOSER_GET_IFACE (self)->get_key_uri (self);
}

/**
 * gcr_certificate_chooser_set_key_password:
 * @self: the #GcrCertificateChooser
 * @key_password:
 */
void
gcr_certificate_chooser_set_key_password (GcrCertificateChooser *self, const gchar *key_password)
{
	g_return_if_fail (GCR_IS_CERTIFICATE_CHOOSER (self));

	GCR_CERTIFICATE_CHOOSER_GET_IFACE (self)->set_key_password (self, key_password);
}

/**
 * gcr_certificate_chooser_get_key_password:
 * @self: the #GcrCertificateChooser
 *
 * Returns: (transfer full):
 */
gchar *
gcr_certificate_chooser_get_key_password (GcrCertificateChooser *self)
{
	g_return_val_if_fail (GCR_IS_CERTIFICATE_CHOOSER (self), NULL);

	return GCR_CERTIFICATE_CHOOSER_GET_IFACE (self)->get_key_password (self);
}
