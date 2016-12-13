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

#include "gcr-tokens-sidebar-row.h"

#include <glib/gi18n.h>
#include <gck/gck.h>

/**
 * SECTION:gcr-tokens-sidebar-row
 * @title: GcrTokensSidebarRow
 * @short_description: The PKCS11 Tokens Sidebar Row
 * @see_also: #GcrTokensSidebarRow
 *
 * #GcrTokensSidebarRow is the tokens sidebar row widget.
 */

enum {
	TOKEN_LOGIN,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _GcrTokensSidebarRowPrivate
{
	GckSlot *slot;
	GtkEventBox *event_box;
	GtkImage *icon_widget;
	GtkLabel *label_widget;
	GtkButton *login_button;
};

G_DEFINE_TYPE_WITH_CODE (GcrTokensSidebarRow, gcr_tokens_sidebar_row, GTK_TYPE_LIST_BOX_ROW,
                         G_ADD_PRIVATE (GcrTokensSidebarRow));

enum
{
	PROP_0,
	PROP_SLOT,
};

static void
get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GcrTokensSidebarRow *self = GCR_TOKENS_SIDEBAR_ROW (object);
	GcrTokensSidebarRowPrivate *priv = self->priv;

	switch (prop_id) {
	case PROP_SLOT:
		if (priv->slot)
			g_value_set_object (value, priv->slot);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GcrTokensSidebarRow *self = GCR_TOKENS_SIDEBAR_ROW (object);
	GcrTokensSidebarRowPrivate *priv = self->priv;
	gboolean needs_login = FALSE;

	switch (prop_id) {
	case PROP_SLOT:
		/* Construct only. */
		priv->slot = g_value_dup_object (value);

		if (priv->slot) {
			GckTokenInfo *info;

			info = gck_slot_get_token_info (priv->slot);
			g_return_if_fail (info);
			gtk_label_set_text (priv->label_widget, info->label);
			needs_login = info->flags & CKF_LOGIN_REQUIRED;
			gck_token_info_free (info);
		} else {
			gtk_widget_hide (GTK_WIDGET (priv->icon_widget));
			gtk_widget_set_no_show_all (GTK_WIDGET (priv->icon_widget), TRUE);
			gtk_label_set_markup (priv->label_widget, _("<b>All tokens</b>"));
		}

		if (!needs_login) {
			gtk_widget_hide (GTK_WIDGET (priv->login_button));
			gtk_widget_set_no_show_all (GTK_WIDGET (priv->login_button), TRUE);
		}

		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
finalize (GObject *object)
{
	GcrTokensSidebarRow *self = GCR_TOKENS_SIDEBAR_ROW (object);
	GcrTokensSidebarRowPrivate *priv = self->priv;

	g_clear_object (&priv->slot);

	G_OBJECT_CLASS (gcr_tokens_sidebar_row_parent_class)->finalize (object);
}


static void
login_clicked (GtkButton *button, gpointer user_data)
{
	GcrTokensSidebarRow *self = GCR_TOKENS_SIDEBAR_ROW (user_data);

	g_signal_emit (self, signals[TOKEN_LOGIN], 0);
}

static void
gcr_tokens_sidebar_row_class_init (GcrTokensSidebarRowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->finalize = finalize;

	/**
	 * GcrTokensSidebarRowClass::token-login:
	 *
	 * Emitted with the login button is clicked.
	 */
	signals[TOKEN_LOGIN] = g_signal_new ("token-login",
		                             G_OBJECT_CLASS_TYPE (object_class),
		                             G_SIGNAL_RUN_FIRST,
		                             0, NULL, NULL,
		                             g_cclosure_marshal_VOID__VOID,
		                             G_TYPE_NONE,
		                             0);

	/**
	 * GcrTokensSidebarRowClass::slot:
	 *
	 * The PKCS#11 slot or %NULL for all slots.
	 */
	g_object_class_install_property (object_class, PROP_SLOT,
		g_param_spec_object ("slot", "Slot", "PKCS#11 Slot",
		                     GCK_TYPE_SLOT,
		                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/gcr/ui/gcr-tokens-sidebar-row.ui");

	gtk_widget_class_bind_template_child_private (widget_class, GcrTokensSidebarRow, event_box);
	gtk_widget_class_bind_template_child_private (widget_class, GcrTokensSidebarRow, icon_widget);
	gtk_widget_class_bind_template_child_private (widget_class, GcrTokensSidebarRow, label_widget);
	gtk_widget_class_bind_template_child_private (widget_class, GcrTokensSidebarRow, login_button);
	gtk_widget_class_bind_template_callback (widget_class, login_clicked);

	gtk_widget_class_set_css_name (widget_class, "row");
}

static void
gcr_tokens_sidebar_row_init (GcrTokensSidebarRow *self)
{
	self->priv = gcr_tokens_sidebar_row_get_instance_private (self);
	gtk_widget_init_template (GTK_WIDGET (self));
}

/**
 * gcr_tokens_sidebar_row_get_slot:
 * @self: The #GcrTokensSidebarRow
 *
 * The slot getter.
 *
 * Returns: the associated slot, or %NULL
 */
GckSlot *
gcr_tokens_sidebar_row_get_slot (GcrTokensSidebarRow *self)
{
	GcrTokensSidebarRowPrivate *priv = self->priv;

	return priv->slot;
}

/**
 * gcr_tokens_sidebar_row_new:
 * @slot: the associated %slot
 *
 * Creates the new #GcrTokensSidebarRow for given slot.
 * If %NULL is given for the slot, then the "All slots" row
 * is created.
 *
 * Returns: the newly created #GcrTokensSidebarRow
 */
GtkWidget *
gcr_tokens_sidebar_row_new (GckSlot *slot)
{
	return GTK_WIDGET (g_object_new (GCR_TYPE_TOKENS_SIDEBAR_ROW,
	                                 "slot", slot, NULL));
}
