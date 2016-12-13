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

#include "gcr-token-login-dialog.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gck/gck.h>

/**
 * SECTION:gcr-token-login-dialog
 * @title: GcrTokenLoginDialog
 * @short_description: The PKCS11 PIN Dialog
 * @see_also: #GcrObjectChooserDialog
 *
 * #GcrTokenLoginDialog asks for the PKCS11 login pin.
 * It enforces the PIN constrains (maximum & minimum length).
 *
 * Used by the #GcrObjectChooserDialog when the #GcrTokensSidebar indicates
 * that the user requested the token to be logged in.
 */

struct _GcrTokenLoginDialogPrivate
{
	GckSlot *slot;
	GckTokenInfo *info;

	GtkEntry *pin_entry;
	GtkCheckButton *remember;
};

G_DEFINE_TYPE_WITH_CODE (GcrTokenLoginDialog, gcr_token_login_dialog, GTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (GcrTokenLoginDialog));

enum
{
	PROP_0,
	PROP_TOKEN_SLOT,
};

/**
 * gcr_token_login_dialog_get_pin_value:
 * @self: The #GcrTokenLoginDialog
 *
 * Returns: the entered PIN
 */

const guchar *
gcr_token_login_dialog_get_pin_value (GcrTokenLoginDialog *self)
{
	GcrTokenLoginDialogPrivate *priv = self->priv;
	GtkEntryBuffer *buffer = gtk_entry_get_buffer (priv->pin_entry);

	return (guchar *) gtk_entry_buffer_get_text (buffer);
}

/**
 * gcr_token_login_dialog_get_pin_length:
 * @self: The #GcrTokenLoginDialog
 *
 * Returns: the PIN length
 */

gulong
gcr_token_login_dialog_get_pin_length (GcrTokenLoginDialog *self)
{
	GcrTokenLoginDialogPrivate *priv = self->priv;
	GtkEntryBuffer *buffer = gtk_entry_get_buffer (priv->pin_entry);

	return gtk_entry_buffer_get_bytes (buffer);
}

/**
 * gcr_token_login_dialog_get_remember_pin:
 * @self: The #GcrTokenLoginDialog
 *
 * Returns: %TRUE if the "Remember PIN" checkbox was checked
 */

gboolean
gcr_token_login_dialog_get_remember_pin (GcrTokenLoginDialog *self)
{
	GcrTokenLoginDialogPrivate *priv = self->priv;

	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->remember));
}

static void
get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GcrTokenLoginDialog *self = GCR_TOKEN_LOGIN_DIALOG (object);
	GcrTokenLoginDialogPrivate *priv = self->priv;

	switch (prop_id) {
	case PROP_TOKEN_SLOT:
		if (priv->slot)
			g_value_set_object (value, priv->slot);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static gboolean
can_activate (GcrTokenLoginDialog *self)
{
	GcrTokenLoginDialogPrivate *priv = self->priv;
	GtkEntryBuffer *buffer = gtk_entry_get_buffer (priv->pin_entry);
	guint len = gtk_entry_buffer_get_length (buffer);

	return len <= priv->info->max_pin_len && len >= priv->info->min_pin_len;
}

static void
set_slot (GcrTokenLoginDialog *self, GckSlot *slot)
{
	GcrTokenLoginDialogPrivate *priv = self->priv;
	gchar *title;

	g_clear_object (&priv->slot);
	if (priv->info)
		gck_token_info_free (priv->info);

	priv->slot = slot;
	priv->info = gck_slot_get_token_info (slot);

	title = g_strdup_printf (_("Enter %s PIN"), priv->info->label);
	gtk_window_set_title (GTK_WINDOW (self), title);
	g_free (title);

	gtk_entry_set_max_length (priv->pin_entry, priv->info->max_pin_len);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT,
	                                   can_activate (self));
}


static void
set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GcrTokenLoginDialog *self = GCR_TOKEN_LOGIN_DIALOG (object);

	switch (prop_id) {
	case PROP_TOKEN_SLOT:
		set_slot (self, g_value_dup_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
finalize (GObject *object)
{
	GcrTokenLoginDialog *self = GCR_TOKEN_LOGIN_DIALOG (object);
	GcrTokenLoginDialogPrivate *priv = self->priv;

	g_clear_object (&priv->slot);
	if (priv->info) {
		gck_token_info_free (priv->info);
		priv->info = NULL;
	}

	G_OBJECT_CLASS (gcr_token_login_dialog_parent_class)->finalize (object);
}

static void
pin_changed (GtkEditable *editable, gpointer user_data)
{
	gtk_dialog_set_response_sensitive (GTK_DIALOG (user_data), GTK_RESPONSE_ACCEPT,
	                                   can_activate (GCR_TOKEN_LOGIN_DIALOG (user_data)));
}


static void
pin_activate (GtkEditable *editable, gpointer user_data)
{
	if (can_activate (GCR_TOKEN_LOGIN_DIALOG (user_data)))
		gtk_dialog_response (GTK_DIALOG (user_data), GTK_RESPONSE_ACCEPT);
}

static void
gcr_token_login_dialog_class_init (GcrTokenLoginDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->finalize = finalize;

	/**
	 * GcrTokenLoginDialog::token-slot:
	 *
	 * Slot that contains the pin for which the dialog requests
	 * the PIN.
	 */
	g_object_class_install_property (object_class, PROP_TOKEN_SLOT,
		g_param_spec_object ("token-slot", "Slot", "Slot containing the Token",
		                     GCK_TYPE_SLOT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gcr/ui/gcr-token-login-dialog.ui");
	gtk_widget_class_bind_template_child_private (widget_class, GcrTokenLoginDialog, pin_entry);
	gtk_widget_class_bind_template_child_private (widget_class, GcrTokenLoginDialog, remember);
	gtk_widget_class_bind_template_callback (widget_class, pin_changed);
	gtk_widget_class_bind_template_callback (widget_class, pin_activate);
}

static void
gcr_token_login_dialog_init (GcrTokenLoginDialog *self)
{
	self->priv = gcr_token_login_dialog_get_instance_private (self);

	gtk_widget_init_template (GTK_WIDGET (self));
}

/**
 * gcr_token_login_dialog_new:
 * @slot: Slot that contains the pin for which the dialog requests the PIN
 *
 * Creates the new PKCS11 login dialog.
 *
 * Returns: the newly created #GcrTokenLoginDialog
 */
GtkWidget *
gcr_token_login_dialog_new (GckSlot *slot)
{
	return GTK_WIDGET (g_object_new (GCR_TYPE_TOKEN_LOGIN_DIALOG,
"use-header-bar", TRUE,
	                   "token-slot", slot,
	                   NULL));
#if 0
GtkDialogFlags flags,
	                       "use-header-bar", !!(flags & GTK_DIALOG_USE_HEADER_BAR),
	                       "title", title,
#endif
}
