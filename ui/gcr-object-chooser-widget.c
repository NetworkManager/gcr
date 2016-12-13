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
#include "gcr-object-chooser-widget.h"
#include "gcr-tokens-sidebar.h"
#include "gcr-token-login-dialog.h"

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gcr/gcr.h>
#include <gck/gck.h>
#include <p11-kit/pkcs11x.h>

/**
 * SECTION:gcr-object-chooser-widget
 * @title: GcrObjectChooserWidget
 * @short_description: The PKCS11 Object Chooser Widget
 * @see_also: #GcrObjectChooser, #GcrObjectChooserWidget
 *
 * #GcrObjectChooserDialog is the PKCS11 object chooser widget.
 */

enum {
	COLUMN_LABEL,
	COLUMN_ISSUER,
	COLUMN_URI,
	COLUMN_SLOT,
	N_COLUMNS
};

struct _GcrObjectChooserWidgetPrivate
{
	GcrTokensSidebar *tokens_sidebar;
	GtkTreeView *objects_view;
	GtkTreeViewColumn *list_name_column;
	GtkCellRenderer *list_name_renderer;
	GtkTreeViewColumn *list_issued_by_column;
	GtkCellRenderer *list_issued_by_renderer;
	GtkLabel *error_label;
	GtkRevealer *error_revealer;

	GtkListStore *store;
	GtkTreeModel *filter;

	GtkWindow *parent_window;
	gchar *uri;
};

static void gcr_object_chooser_iface_init (GcrObjectChooserInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GcrObjectChooserWidget, gcr_object_chooser_widget, GTK_TYPE_BOX,
                         G_ADD_PRIVATE (GcrObjectChooserWidget)
                         G_IMPLEMENT_INTERFACE (GCR_TYPE_OBJECT_CHOOSER, gcr_object_chooser_iface_init));

enum
{
	PROP_0,
	PROP_PARENT_WINDOW,
};

typedef struct {
	GcrObjectChooserWidget *self;
	GckSlot *slot;
	int remaining;
	guchar *pin_value;
	gulong pin_length;
	gboolean remember_pin;
} ObjectIterData;

static void
object_iter_data_maybe_free (ObjectIterData *data)
{
	if (data->remaining == 0) {
		g_object_unref (data->self);
		if (data->pin_value)
			g_free (data->pin_value);
		g_slice_free (ObjectIterData, data);
	}
}

static void
remove_objects_from_slot (GcrObjectChooserWidget *self, GckSlot *slot)
{
	GcrObjectChooserWidgetPrivate *priv = self->priv;
	GtkTreeIter iter;
	GckSlot *row_slot;
	gboolean more;

	more = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter);
	while (more) {
		gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter, COLUMN_SLOT, &row_slot, -1);
		if (slot == row_slot)
			more = gtk_list_store_remove (priv->store, &iter);
		else
			more = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter);
	}
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
}

static void
object_details (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	GckObject *object = GCK_OBJECT (source_object);
	ObjectIterData *data = user_data;
	GcrObjectChooserWidget *self = data->self;
	GcrObjectChooserWidgetPrivate *priv = self->priv;
	GckAttributes *attrs;
	GtkTreeIter iter;
	gulong category;
	const GckAttribute *attr;
	GtkTreePath *path;
	GcrCertificate *cert;
	gchar *label, *issuer, *uri, *real_uri;
	GckUriData uri_data = { 0, };
	GError *error = NULL;

	attrs = gck_object_get_finish (object, res, &error);
	if (!attrs) {
		/* No better idea than to just ignore the object. */
		g_warning ("Error getting attributes: %s\n", error->message);
		g_error_free (error);
		goto out;
	}

	/* Ignore the uninteresting objects.
	 * XXX: There's more to ignore... */
	if (gck_attributes_find_ulong (attrs, CKA_CERTIFICATE_CATEGORY, &category)) {
		if (category == 2) /* Authority */
			goto out;
		if (category == 0) /* Other -- distrust */
			goto out;
	}

	attr = gck_attributes_find (attrs, CKA_VALUE);
	if (attr && attr->value && attr->length) {
		cert = gcr_simple_certificate_new (attr->value, attr->length);
		label = gcr_certificate_get_subject_name (cert);
		issuer = gcr_certificate_get_issuer_name (cert);

		uri_data.attributes = attrs;
		uri_data.token_info = gck_slot_get_token_info (data->slot);
		uri = gck_uri_build (&uri_data, GCK_URI_FOR_OBJECT_ON_TOKEN);

		if (data->remember_pin) {
			/* XXX: We're assuming that the URI has no attributes. That is the
			 * case for Gck now, but may change in future? */
			real_uri = g_strdup_printf ("%s&pin-value=%s", uri, data->pin_value);
			g_free (uri);
			uri = real_uri;
		}

		gtk_list_store_append (priv->store, &iter);

		/* Ignore the attributes, such as PIN. */
		if (   priv->uri && g_str_has_prefix (priv->uri, uri)
		    && (priv->uri[strlen(uri)] == '&' || priv->uri[strlen(uri)] == '\0')) {
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), &iter);
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->objects_view), path, NULL, FALSE);
			gtk_tree_path_free (path);
			g_free (uri);
			uri = g_strdup (priv->uri);
		}

		gtk_list_store_set (priv->store, &iter,
		                    COLUMN_LABEL, label,
		                    COLUMN_ISSUER, issuer,
		                    COLUMN_URI, uri,
		                    COLUMN_SLOT, data->slot,
		                    -1);

		gck_token_info_free (uri_data.token_info);
		g_free (label);
		g_free (issuer);
		g_free (uri);
		g_object_unref (cert);
	}

out:
	if (attrs)
		gck_attributes_unref (attrs);
	data->remaining--;
	object_iter_data_maybe_free (data);
}

static void
next_object (GObject *obj, GAsyncResult *res, gpointer user_data)
{
	GckEnumerator *enm = GCK_ENUMERATOR (obj);
	ObjectIterData *data = user_data;
	GList *objects;
	GList *iter;
	GError *error = NULL;

	objects = gck_enumerator_next_finish (enm, res, &error);
	if (error) {
		/* Again, no better idea than to just ignore the object. */
		g_warning ("Error getting object: %s", error->message);
		g_error_free (error);
		goto out;
	}

	for (iter = objects; iter; iter = iter->next) {
		GckObject *object = GCK_OBJECT (iter->data);
		const gulong attr_types[] = { CKA_ID, CKA_LABEL, CKA_ISSUER,
		                              CKA_VALUE, CKA_CERTIFICATE_CATEGORY };

		data->remaining++;
		gck_object_get_async (object, attr_types,
		                      sizeof(attr_types) / sizeof(attr_types[0]),
		                      NULL, object_details, data);
	}

	gck_list_unref_free (objects);
out:
	object_iter_data_maybe_free (data);
}

static void
reload_slot (GckSession *session, ObjectIterData *data)
{
	GckBuilder *builder;
	GckEnumerator *enm;

	remove_objects_from_slot (data->self, data->slot);
	builder = gck_builder_new (GCK_BUILDER_NONE);
	gck_builder_add_ulong (builder, CKA_CLASS, CKO_CERTIFICATE);
	enm = gck_session_enumerate_objects (session, gck_builder_end (builder));
	gck_enumerator_next_async (enm, -1, NULL, next_object, data);
}

static void
logged_in (GObject *obj, GAsyncResult *res, gpointer user_data)
{
	GckSession *session = GCK_SESSION (obj);
	ObjectIterData *data = user_data;
	GcrObjectChooserWidget *self = data->self;
	GcrObjectChooserWidgetPrivate *priv = self->priv;
	GError *error = NULL;

	if (!gck_session_login_finish (session, res, &error)) {
		g_prefix_error (&error, _("Error logging in: "));
		gtk_label_set_label (priv->error_label, error->message);
		gtk_revealer_set_reveal_child (priv->error_revealer, TRUE);
		g_error_free (error);
		object_iter_data_maybe_free (data);
		return;
	}

	reload_slot (session, data);
	g_clear_object (&session);
}

static void
session_opened (GObject *obj, GAsyncResult *res, gpointer user_data)
{
	ObjectIterData *data = user_data;
	GckSession *session;
	GError *error = NULL;

	session = gck_slot_open_session_finish (data->slot, res, &error);
	if (error) {
		/* It probably doesn't make sense to tell the user, since
		 * there's not much they could do about it. It could be that
		 * this occured on the initial token scan for multiple tokens
		 * and the errors would displace the other one anyway. */
		g_warning ("Error opening a session: %s", error->message);
		g_error_free (error);
		object_iter_data_maybe_free (data);
		return;
	}

	if (data->pin_value) {
		gck_session_login_async (session, CKU_USER,
		                         data->pin_value, data->pin_length,
		                         NULL, logged_in, data);
	} else {
		reload_slot (session, data);
		g_clear_object (&session);
	}
}

static void
token_login (GcrTokensSidebar *tokens_sidebar, GckSlot *slot, gpointer user_data)
{
	GcrObjectChooserWidget *self = user_data;
	GcrObjectChooserWidgetPrivate *priv = self->priv;
	GtkWidget *dialog;
	ObjectIterData *data;
	GckTokenInfo *token_info;
	gboolean has_pin_pad = FALSE;

	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));

	/* See if the token has a PIN pad. */
	token_info = gck_slot_get_token_info (slot);
	if (token_info->flags & CKF_PROTECTED_AUTHENTICATION_PATH)
		has_pin_pad = TRUE;
	gck_token_info_free (token_info);

	if (has_pin_pad) {
		/* Login with empty credentials makes the token
		 * log in on its PIN pad. */
		data = g_slice_alloc0 (sizeof (ObjectIterData));
		data->self = g_object_ref (self);
		data->slot = slot;
		data->pin_length = 0;
		data->pin_value =  g_memdup ("", 1);
		data->remember_pin = TRUE;
		gck_slot_open_session_async (slot, GCK_SESSION_READ_ONLY, NULL, session_opened, data);
		return;
	}

	/* The token doesn't have a PIN pad. Ask for PIN. */
        dialog = gcr_token_login_dialog_new (slot);
	gtk_window_set_transient_for (GTK_WINDOW (dialog), priv->parent_window);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		data = g_slice_alloc0 (sizeof (ObjectIterData));
		data->self = g_object_ref (self);
		data->slot = slot;
		data->pin_length = gcr_token_login_dialog_get_pin_length (GCR_TOKEN_LOGIN_DIALOG (dialog));
		data->pin_value = g_memdup (gcr_token_login_dialog_get_pin_value (GCR_TOKEN_LOGIN_DIALOG (dialog)),
		                            data->pin_length + 1);
		data->remember_pin = gcr_token_login_dialog_get_remember_pin (GCR_TOKEN_LOGIN_DIALOG (dialog));
		gck_slot_open_session_async (slot, GCK_SESSION_READ_ONLY, NULL, session_opened, data);
	}

	gtk_widget_destroy (dialog);
}

static void
token_added (GcrTokensSidebar *tokens_sidebar, GckSlot *slot, gpointer user_data)
{
	GcrObjectChooserWidget *self = user_data;
	ObjectIterData *data;

	data = g_slice_alloc0 (sizeof (ObjectIterData));
	data->self = g_object_ref (self);
	data->slot = slot;
	gck_slot_open_session_async (slot, GCK_SESSION_READ_ONLY, NULL, session_opened, data);
}

static void
open_token (GcrTokensSidebar *tokens_sidebar, GckSlot *slot, gpointer user_data)
{
	GcrObjectChooserWidget *self = user_data;
	GcrObjectChooserWidgetPrivate *priv = self->priv;

	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
}

static void
set_uri (GcrObjectChooser *self, const gchar *uri)
{
	GcrObjectChooserWidgetPrivate *priv = GCR_OBJECT_CHOOSER_WIDGET (self)->priv;

	if (priv->uri)
		g_free (priv->uri);
	priv->uri = g_strdup (uri);
}

static gchar *
get_uri (GcrObjectChooser *chooser)
{
	GcrObjectChooserWidget *self = GCR_OBJECT_CHOOSER_WIDGET (chooser);
	GcrObjectChooserWidgetPrivate *priv = self->priv;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *uri;

	gtk_tree_view_get_cursor (priv->objects_view, &path, NULL);
	if (path == NULL)
		return NULL;

	if (!gtk_tree_model_get_iter (priv->filter, &iter, path))
		g_return_val_if_reached (NULL);

	gtk_tree_model_get (priv->filter, &iter, COLUMN_URI, &uri, -1);

	return uri;
}

static void
row_activated (GtkTreeView *tree_view, GtkTreePath *path,
               GtkTreeViewColumn *column, gpointer user_data)
{
	GcrObjectChooserWidget *self = user_data;

	g_signal_emit_by_name (self, "object-activated");
}

static void
cursor_changed (GtkTreeView *tree_view, gpointer user_data)
{
	GcrObjectChooserWidget *self = user_data;

	g_signal_emit_by_name (self, "object-selected");
}


static gboolean
slot_visible (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	GcrObjectChooserWidget *self = data;
	GcrObjectChooserWidgetPrivate *priv = self->priv;
	GckSlot *active_slot;

	active_slot = gcr_tokens_sidebar_get_slot (priv->tokens_sidebar);
	if (active_slot) {
		GckSlot *slot;

		gtk_tree_model_get (model, iter, COLUMN_SLOT, &slot, -1);
		if (slot)
			g_object_unref (slot);
		return slot == active_slot;
	} else {
		/* All tokens. */
		return TRUE;
	}
}

static void
error_close (GtkInfoBar *bar, gint response_id, gpointer user_data)
{
	GcrObjectChooserWidget *self = user_data;
	GcrObjectChooserWidgetPrivate *priv = self->priv;

	gtk_revealer_set_reveal_child (priv->error_revealer, FALSE);
}

static void
get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GcrObjectChooserWidget *self = GCR_OBJECT_CHOOSER_WIDGET (object);
	GcrObjectChooserWidgetPrivate *priv = self->priv;

	switch (prop_id) {
	case PROP_PARENT_WINDOW:
		if (priv->parent_window)
			g_value_set_object (value, priv->parent_window);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GcrObjectChooserWidget *self = GCR_OBJECT_CHOOSER_WIDGET (object);
	GcrObjectChooserWidgetPrivate *priv = self->priv;

	switch (prop_id) {
	case PROP_PARENT_WINDOW:
		priv->parent_window = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
finalize (GObject *object)
{
	GcrObjectChooserWidget *self = GCR_OBJECT_CHOOSER_WIDGET (object);
	GcrObjectChooserWidgetPrivate *priv = self->priv;

	g_clear_object (&priv->store);
	g_clear_object (&priv->filter);
	g_clear_object (&priv->parent_window);

	if (priv->uri)
		g_free (priv->uri);

	G_OBJECT_CLASS (gcr_object_chooser_widget_parent_class)->finalize (object);
}

static void
gcr_object_chooser_iface_init (GcrObjectChooserInterface *iface)
{
	iface->set_uri = set_uri;
	iface->get_uri = get_uri;
}

static void
gcr_object_chooser_widget_class_init (GcrObjectChooserWidgetClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->finalize = finalize;

	g_object_class_install_property (object_class, PROP_PARENT_WINDOW,
		g_param_spec_object ("parent-window", "Parent Window", "Window to which to attach login idalogs",
		                     GTK_TYPE_WINDOW, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_type_ensure (GCR_TYPE_TOKENS_SIDEBAR);
	gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gcr/ui/gcr-object-chooser-widget.ui");

	gtk_widget_class_bind_template_child_private (widget_class, GcrObjectChooserWidget, tokens_sidebar);
	gtk_widget_class_bind_template_child_private (widget_class, GcrObjectChooserWidget, objects_view);
	gtk_widget_class_bind_template_child_private (widget_class, GcrObjectChooserWidget, list_name_column);
	gtk_widget_class_bind_template_child_private (widget_class, GcrObjectChooserWidget, list_name_renderer);
	gtk_widget_class_bind_template_child_private (widget_class, GcrObjectChooserWidget, list_issued_by_column);
	gtk_widget_class_bind_template_child_private (widget_class, GcrObjectChooserWidget, list_issued_by_renderer);
	gtk_widget_class_bind_template_child_private (widget_class, GcrObjectChooserWidget, error_revealer);
	gtk_widget_class_bind_template_child_private (widget_class, GcrObjectChooserWidget, error_label);
	gtk_widget_class_bind_template_callback (widget_class, token_added);
	gtk_widget_class_bind_template_callback (widget_class, token_login);
	gtk_widget_class_bind_template_callback (widget_class, open_token);
	gtk_widget_class_bind_template_callback (widget_class, row_activated);
	gtk_widget_class_bind_template_callback (widget_class, cursor_changed);
	gtk_widget_class_bind_template_callback (widget_class, error_close);
}

static void
gcr_object_chooser_widget_init (GcrObjectChooserWidget *self)
{
	GcrObjectChooserWidgetPrivate *priv;

	self->priv = gcr_object_chooser_widget_get_instance_private (self);
	priv = self->priv;

	gtk_widget_init_template (GTK_WIDGET (self));

	gtk_tree_view_column_set_attributes (priv->list_name_column, priv->list_name_renderer, "text", 0, NULL);
	gtk_tree_view_column_set_attributes (priv->list_issued_by_column, priv->list_issued_by_renderer, "text", 1, NULL);

	priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GCK_TYPE_SLOT);
	priv->filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->store), NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filter), slot_visible, self, NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->objects_view), GTK_TREE_MODEL (priv->filter));
	gtk_widget_set_size_request (GTK_WIDGET (priv->objects_view), 380, -1);
}

/**
 * gcr_object_chooser_widget_new:
 * @parent_window: (allow-none): The parent window or %NULL
 *
 * Creates the new #GcrObjectChooserWidget. The optional parent window
 * will be used to attach the PIN prompts to.
 *
 * Returns: newly created #GcrObjectChooserWidget
 */

GtkWidget *
gcr_object_chooser_widget_new (GtkWindow *parent_window)
{
	return GTK_WIDGET (g_object_new (GCR_TYPE_OBJECT_CHOOSER_WIDGET,
	                                 "parent-window", parent_window,
	                                 NULL, NULL));
}
