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

#include "gcr-object-chooser-dialog.h"
#include "gcr-object-chooser-widget.h"

#include <gtk/gtk.h>

/**
 * SECTION:gcr-object-chooser-dialog
 * @title: GcrObjectChooserDialog
 * @short_description: The PKCS11 Object Chooser Dialog
 * @see_also: #GcrObjectChooser
 *
 * #GcrObjectChooserDialog is the PKCS11 object chooser dialog.
 */

struct _GcrObjectChooserDialogPrivate
{
	GcrObjectChooserWidget *chooser_widget;
};

static void gcr_object_chooser_iface_init (GcrObjectChooserInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GcrObjectChooserDialog, gcr_object_chooser_dialog, GTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (GcrObjectChooserDialog)
                         G_IMPLEMENT_INTERFACE (GCR_TYPE_OBJECT_CHOOSER, gcr_object_chooser_iface_init));

static void
object_selected (GcrObjectChooser *chooser, GcrObjectChooserDialog *self)
{
	GcrObjectChooserDialogPrivate *priv = GCR_OBJECT_CHOOSER_DIALOG (self)->priv;
	gchar *uri;

	uri = gcr_object_chooser_get_uri (GCR_OBJECT_CHOOSER (priv->chooser_widget));
	gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT, uri != NULL);
	g_free (uri);
}

static void
object_activated (GcrObjectChooser *chooser, GcrObjectChooserDialog *self)
{
	if (gtk_window_activate_default (GTK_WINDOW (self)))
		return;
}

static void
set_uri (GcrObjectChooser *self, const gchar *uri)
{
	GcrObjectChooserDialogPrivate *priv = GCR_OBJECT_CHOOSER_DIALOG (self)->priv;

	return gcr_object_chooser_set_uri (GCR_OBJECT_CHOOSER (priv->chooser_widget), uri);
}

static gchar *
get_uri (GcrObjectChooser *self)
{
	GcrObjectChooserDialogPrivate *priv = GCR_OBJECT_CHOOSER_DIALOG (self)->priv;

	return gcr_object_chooser_get_uri (GCR_OBJECT_CHOOSER (priv->chooser_widget));
}

static void
gcr_object_chooser_iface_init (GcrObjectChooserInterface *iface)
{
	iface->set_uri = set_uri;
	iface->get_uri = get_uri;
}

static void
gcr_object_chooser_dialog_class_init (GcrObjectChooserDialogClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_ensure (GCR_TYPE_OBJECT_CHOOSER_WIDGET);
	gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gcr/ui/gcr-object-chooser-dialog.ui");

	gtk_widget_class_bind_template_child_private (widget_class, GcrObjectChooserDialog, chooser_widget);
	gtk_widget_class_bind_template_callback (widget_class, object_selected);
	gtk_widget_class_bind_template_callback (widget_class, object_activated);
}

static void
gcr_object_chooser_dialog_init (GcrObjectChooserDialog *self)
{
	GcrObjectChooserDialogPrivate *priv;

	self->priv = gcr_object_chooser_dialog_get_instance_private (self);
	priv = self->priv;

	gtk_widget_init_template (GTK_WIDGET (self));
	g_object_set (G_OBJECT (priv->chooser_widget), "parent-window", self, NULL);
	gtk_widget_show_all (GTK_WIDGET (priv->chooser_widget));
}

static GtkWidget *
gcr_object_chooser_dialog_new_valist (const gchar *title, GtkWindow *parent,
                                      GtkDialogFlags flags,
                                      const gchar *first_button_text,
                                      va_list varargs)
{
	GtkWidget *self;
	const char *button_text = first_button_text;
	gint response_id;

	self = g_object_new (GCR_TYPE_OBJECT_CHOOSER_DIALOG,
	                       "use-header-bar", !!(flags & GTK_DIALOG_USE_HEADER_BAR),
	                       "title", title,
	                       NULL);

	if (parent)
		gtk_window_set_transient_for (GTK_WINDOW (self), parent);

	while (button_text) {
		response_id = va_arg (varargs, gint);
		gtk_dialog_add_button (GTK_DIALOG (self), button_text, response_id);
		button_text = va_arg (varargs, const gchar *);
	}

	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT, FALSE);

	return self;
}

/**
 * gcr_object_chooser_dialog_new:
 * @title: The dialog window title
 * @parent: (allow-none): The parent window or %NULL
 * @flags: The dialog flags
 * @first_button_text: (allow-none): The text of the first button
 * @...: response ID for the first button, texts and response ids for other buttons, terminated with %NULL
 *
 * Creates the new #GcrObjectChooserDialog.
 *
 * Returns: newly created #GcrObjectChooserDialog
 */

GtkWidget *
gcr_object_chooser_dialog_new (const gchar *title, GtkWindow *parent,
                               GtkDialogFlags flags,
                               const gchar *first_button_text, ...)
{
	GtkWidget *result;
	va_list varargs;

	va_start (varargs, first_button_text);
	result = gcr_object_chooser_dialog_new_valist (title, parent, flags,
	                                               first_button_text,
	                                               varargs);
	va_end (varargs);

	return result;
}
