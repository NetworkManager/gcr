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

#include "gcr-tokens-sidebar.h"
#include "gcr-tokens-sidebar-row.h"

#include <gck/gck.h>

/**
 * SECTION:gcr-tokens-sidebar
 * @title: GcrTokensSidebar
 * @short_description: The PKCS11 Tokens Sidebar
 * @see_also: #GcrObjectChooserDialog
 *
 * #GcrTokensSidebar is the list of selectable PKCS11 slots present in the
 * system. The slots that support PIN have a login button.
 */

enum {
	TOKEN_ADDED,
	TOKEN_LOGIN,
	OPEN_TOKEN,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _GcrTokensSidebarPrivate
{
	GtkWidget *viewport;
	GtkWidget *listbox;
};

G_DEFINE_TYPE_WITH_CODE (GcrTokensSidebar, gcr_tokens_sidebar, GTK_TYPE_SCROLLED_WINDOW,
                         G_ADD_PRIVATE (GcrTokensSidebar));


static void
token_login (GcrTokensSidebarRow *row, gpointer user_data)
{
	GcrTokensSidebar *self = user_data;
	GcrTokensSidebarPrivate *priv = self->priv;

	gtk_list_box_select_row (GTK_LIST_BOX (priv->listbox), GTK_LIST_BOX_ROW (row));

	g_signal_emit (self, signals[TOKEN_LOGIN], 0,
	               gcr_tokens_sidebar_row_get_slot (row));
}

static void
gcr_tokens_sidebar_add_slot (GcrTokensSidebar *self, GckSlot *slot)
{
	GcrTokensSidebarPrivate *priv = self->priv;
	GtkWidget *row;

	row = gcr_tokens_sidebar_row_new (slot);
	g_signal_connect (row, "token-login", G_CALLBACK (token_login), self);

	gtk_container_add (GTK_CONTAINER (priv->listbox), row);
	gtk_widget_show_all (row);
}

static void
modules_initialized (GObject *object, GAsyncResult *res, gpointer user_data)
{
	GcrTokensSidebar *self = GCR_TOKENS_SIDEBAR (user_data);
	GList *slots;
	GList *iter;
	GError *error = NULL;
	GList *modules;

	modules = gck_modules_initialize_registered_finish (res, &error);
	if (!modules) {
		/* The Front Fell Off. */
		g_critical ("Error getting registered modules: %s", error->message);
		g_error_free (error);
	}

	slots = gck_modules_get_slots (modules, FALSE);

	for (iter = slots; iter; iter = iter->next) {
		GckSlot *slot = GCK_SLOT (iter->data);

		g_signal_emit (self, signals[TOKEN_ADDED], 0, slot);
		gcr_tokens_sidebar_add_slot (self, slot);
	}

	gck_list_unref_free (slots);
	gck_list_unref_free (modules);
}

static void
gcr_tokens_sidebar_class_init (GcrTokensSidebarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	gtk_widget_class_set_css_name (widget_class, "placessidebar");

	/**
	 * GcrTokensSidebar::token-added:
	 * @slot: The slot that is populated with a token
	 *
	 * Emmitted when a token was found in a slot.
	 * The #GcrTokensSidebar initiates a search for tokens when
	 * constructed and emits this signal for each slot.
	 */
	signals[TOKEN_ADDED] = g_signal_new ("token-added",
		                            G_OBJECT_CLASS_TYPE (object_class),
		                            G_SIGNAL_RUN_FIRST,
		                            0, NULL, NULL,
		                            g_cclosure_marshal_VOID__OBJECT,
		                            G_TYPE_NONE,
		                            1, G_TYPE_OBJECT);

	/**
	 * GcrTokensSidebar::token-login:
	 * @slot: The slot for which login was requested
	 *
	 * Emmitted when the login button was clocked, requesting
	 * the entry of a PIN.
	 */
	signals[TOKEN_LOGIN] = g_signal_new ("token-login",
		                            G_OBJECT_CLASS_TYPE (object_class),
		                            G_SIGNAL_RUN_FIRST,
		                            0, NULL, NULL,
		                            g_cclosure_marshal_VOID__OBJECT,
		                            G_TYPE_NONE,
		                            1, G_TYPE_OBJECT);

	/**
	 * GcrTokensSidebar::open-token:
	 * @slot: The slot that contains the selected token
	 *
	 * Emmitted when the token was selected.
	 */
	signals[OPEN_TOKEN] = g_signal_new ("open-token",
		                            G_OBJECT_CLASS_TYPE (object_class),
		                            G_SIGNAL_RUN_FIRST,
		                            0, NULL, NULL,
		                            g_cclosure_marshal_VOID__OBJECT,
		                            G_TYPE_NONE,
		                            1, G_TYPE_OBJECT);
}

static void
row_activated (GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	GcrTokensSidebar *self = user_data;

	g_signal_emit (self, signals[OPEN_TOKEN], 0,
	               gcr_tokens_sidebar_row_get_slot (GCR_TOKENS_SIDEBAR_ROW (row)));
}

static void
gcr_tokens_sidebar_init (GcrTokensSidebar *self)
{
	GcrTokensSidebarPrivate *priv;

	self->priv = gcr_tokens_sidebar_get_instance_private (self);
	priv = self->priv;

	gtk_widget_set_size_request (GTK_WIDGET (self), 260, 280);

	priv->viewport = gtk_viewport_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (self), priv->viewport);

	priv->listbox = gtk_list_box_new ();
	gtk_container_add (GTK_CONTAINER (priv->viewport), priv->listbox);

	g_signal_connect (priv->listbox, "row-activated", G_CALLBACK (row_activated), self);

	gcr_tokens_sidebar_add_slot (self, NULL);
	gck_modules_initialize_registered_async (NULL, modules_initialized, self);
}

/**
 * gcr_tokens_sidebar_get_slot:
 * @self: The #GcrTokensSidebar
 *
 * Gets the currently selected slot.
 *
 * Returns: the currently selected slot of %NULL if "All tokens" are selected.
 */
GckSlot *
gcr_tokens_sidebar_get_slot (GcrTokensSidebar *self)
{
	GcrTokensSidebarPrivate *priv = self->priv;
	GtkListBoxRow *row = gtk_list_box_get_selected_row (GTK_LIST_BOX (priv->listbox));

	if (row == NULL)
		return NULL;

	return gcr_tokens_sidebar_row_get_slot (GCR_TOKENS_SIDEBAR_ROW (row));
}

/**
 * gcr_tokens_sidebar_new:
 *
 * Creates the new Tokens sidebar.
 *
 * Returns: the newly created #GcrTokensSidebar
 */
GtkWidget *
gcr_tokens_sidebar_new ()
{
	return GTK_WIDGET (g_object_new (GCR_TYPE_TOKENS_SIDEBAR, NULL));
}
