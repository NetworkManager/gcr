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

#ifndef __GCR_CERTIFICATE_CHOOSER_H__
#define __GCR_CERTIFICATE_CHOOSER_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GcrCertificateChooser GcrCertificateChooser;

struct _GcrCertificateChooserInterface {
	GTypeInterface g_iface;

	void (*set_cert_uri) (GcrCertificateChooser *self, const gchar *cert_uri);
	gchar *(*get_cert_uri) (GcrCertificateChooser *self);

	void (*set_cert_password) (GcrCertificateChooser *self, const gchar *cert_password);
	gchar *(*get_cert_password) (GcrCertificateChooser *self);

	void (*set_key_uri) (GcrCertificateChooser *self, const gchar *key_uri);
	gchar *(*get_key_uri) (GcrCertificateChooser *self);

	void (*set_key_password) (GcrCertificateChooser *self, const gchar *key_password);
	gchar *(*get_key_password) (GcrCertificateChooser *self);
};

#define GCR_TYPE_CERTIFICATE_CHOOSER gcr_certificate_chooser_get_type ()
G_DECLARE_INTERFACE (GcrCertificateChooser, gcr_certificate_chooser, GCR, CERTIFICATE_CHOOSER, GObject)

void gcr_certificate_chooser_set_cert_uri (GcrCertificateChooser *self, const gchar *cert_uri);
gchar *gcr_certificate_chooser_get_cert_uri (GcrCertificateChooser *self);

void gcr_certificate_chooser_set_cert_password (GcrCertificateChooser *self, const gchar *cert_password);
gchar *gcr_certificate_chooser_get_cert_password (GcrCertificateChooser *self);

void gcr_certificate_chooser_set_key_uri (GcrCertificateChooser *self, const gchar *key_uri);
gchar *gcr_certificate_chooser_get_key_uri (GcrCertificateChooser *self);

void gcr_certificate_chooser_set_key_password (GcrCertificateChooser *self, const gchar *key_password);
gchar *gcr_certificate_chooser_get_key_password (GcrCertificateChooser *self);

G_END_DECLS

#endif /* __GCR_CERTIFICATE_CHOOSER_H__ */
