/*
 * gnome-keyring
 *
 * Copyright (C) 2008 Stefan Walter
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
 */

#include "config.h"

#include "gcr/gcr-icons.h"
#include "gcr/gcr-parser.h"
#include "gcr/gcr.h"
#include "gcr-dialog-util.h"
#include "gcr-secure-entry-buffer.h"
#include "gcr-certificate-chooser-dialog.h"
#include "gcr-certificate-chooser-pkcs11.c"
#include "gcr-viewer.h"
#include "gcr-viewer-widget.h"
#include "gcr-unlock-renderer.h"
#include "egg/egg-secure-memory.h"
#include <gtk/gtk.h>
#include "gcr-failure-renderer.h"

#include <glib/gi18n-lib.h>
#include <gdk/gdkx.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
/**
 * SECTION:gcr-certificate-chooser-dialog
 * @title: GcrCertificateChooserDialog
 * @short_description: A dialog which allows selection of personal certificates
 *
 * A dialog which guides the user through selection of a certificate and
 * corresponding private key, located in files or PKCS\#11 tokens.
 */

/**
 * GcrCertificateChooserDialog:
 *
 * A certificate chooser dialog object.
 */

/**
 * GcrCertificateChooserDialogClass:
 *
 * Class for #GcrCertificateChooserDialog
 */
#define GCR_CERTIFICATE_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GCR_TYPE_CERTIFICATE_CHOOSER_DIALOG, GcrCertificateChooserDialogClass))
#define GCR_IS_CERTIFICATE_CHOOSER_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), GCR_TYPE_CERTIFICATE_CHOOSER_DIALOG))
#define GCR_CERTIFICATE_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GCR_TYPE_CERTIFICATE_CHOOSER_DIALOG, GcrCertificateChooserDialogClass))

#define GCR_CERTIFICATE_CHOOSER_SIDEBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GCR_TYPE_CERTIFICATE_CHOOSER_SIDEBAR, GcrCertificateChooserSidebarClass))
#define GCR_IS_CERTIFICATE_CHOOSER_SIDEBAR_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), GCR_TYPE_CERTIFICATE_CHOOSER_SIDEBAR))
#define GCR_CERTIFICATE_CHOOSER_SIDEBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GCR_TYPE_CERTIFICATE_CHOOSER_SIDEBAR, GcrCertificateChooserSidebarClass))



enum {
	TOKEN_UPDATE,
	TOKEN_ADDED
};

static gint certificate_chooser_signals[TOKEN_ADDED + 1] = { 0 };

struct _GcrCertificateChooserDialog {
       GtkDialog parent;
       GList *slots;
       GtkListStore *store;
       GList *blacklist;
       gboolean loaded;
       GtkBuilder *builder;
       GcrViewer *page1_viewer;
       GcrViewer *page2_viewer;
       GtkWidget *hbox;
       GBytes *data;
       GcrCertificateChooserSidebar *page1_sidebar;
       GcrCertificateChooserSidebar *page2_sidebar;
       char *certificate_uri;
       char *key_uri;
       GcrParser *parser;
       gboolean is_certificate_choosen;
       gboolean is_key_choosen;
       gint password_wrong_count;
};

struct _GcrCertificateChooserSidebar {
       GtkScrolledWindow parent;
       GtkWidget *tree_view;
};

struct _GcrCertificateChooserSidebarClass {
       GtkScrolledWindowClass parent_class;
};

typedef struct _GcrCertificateChooserDialogClass GcrCertificateChooserDialogClass;
typedef struct _GcrCertificateChooserSidebarClass GcrCertificateChooserSidebarClass;

struct _GcrCertificateChooserDialogClass {
	GtkDialogClass parent;

        /* Signals */
        
         void (*token_added) (GcrCertificateChooserDialog *sig);
};

static const char *token_blacklist[] = { 
        "pkcs11:manufacturer=Gnome%20Keyring;serial=1:SSH:HOME",
        "pkcs11:manufacturer=Gnome%20Keyring;serial=1:SECRET:MAIN",
        "pkcs11:manufacturer=Mozilla%20Foundation;token=NSS%20Generic%20Crypto%20Services",
         NULL
};

static const gchar *file_system = "file-system";

static const char *pkcs11_token = "pkcs11-token";

static const char *no_key = "No key selected yet";

static const char *no_certificate = "No certificate selected yet";

enum {
        //ROW_TYPE,
        TOKEN_LABEL
};
        

enum {
        ROW_TYPE,
        COLUMN_SLOT,
        N_COLUMNS
};

G_DEFINE_TYPE (GcrCertificateChooserDialog, gcr_certificate_chooser_dialog, GTK_TYPE_DIALOG);
G_DEFINE_TYPE (GcrCertificateChooserSidebar, gcr_certificate_chooser_sidebar, GTK_TYPE_SCROLLED_WINDOW);

static void
gcr_certificate_chooser_sidebar_init (GcrCertificateChooserSidebar *self)
{
        
}

static void 
get_token_label (GtkTreeViewColumn *column,
                 GtkCellRenderer *cell,
                 GtkTreeModel *model,
                 GtkTreeIter *iter,
                 gpointer user_data)
{

        GckSlot *slot;
        gchar *label;
        GckTokenInfo *info;
        gtk_tree_model_get (model, iter, ROW_TYPE, &label, COLUMN_SLOT, &slot, -1);
        if (!slot) {

                 g_object_set(cell, 
                              "visible", TRUE,
                              "text", file_system,
                              NULL);
        } else {
 
		//          gtk_tree_model_get (model, iter, COLUMN_SLOT, &slot, -1);
        
                 if (slot == NULL)
                       return ;
                 info = gck_slot_get_token_info (slot);
                 g_object_set(cell, 
                              "visible", TRUE,
                              "text", info->label,
                              NULL);
        }
}

static void
on_page1_tree_node_select (GtkTreeModel *model,
                     GtkTreePath *path,
                     GtkTreeIter *iter,
                     gpointer user_data)
{

        GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG(user_data);
        GckSlot *slot;
        gchar *label;
        GtkWidget *previous_child;
        GcrCertificateChooserPkcs11 *token;

        gtk_tree_model_get (model, iter, ROW_TYPE, &label, COLUMN_SLOT, &slot, -1);
        gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(
                           self->builder, "key-label")),
                           no_key);
        
        gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(
                           self->builder, "certificate-label")),
                           no_certificate);
       
        previous_child =  gtk_paned_get_child2(GTK_PANED(gtk_builder_get_object(
                                               self->builder, "page1-box")));
        

        if (previous_child != NULL) {

                 if (!GTK_IS_FILE_CHOOSER (previous_child)) {

                          
                          token = GCR_CERTIFICATE_CHOOSER_PKCS11 (previous_child);
                          gck_session_logout (token->session,
                                              NULL,
                                              NULL);
                          gtk_widget_destroy (previous_child);
                 } else {
                 
                          gtk_container_remove (GTK_CONTAINER (gtk_builder_get_object(
                                                self->builder, "page1-box")),
                                                previous_child);
                 }
        }

        if (!slot) {
        
                gtk_paned_pack2(GTK_PANED(gtk_builder_get_object(self->builder, "page1-box")), 
                                GTK_WIDGET (gtk_builder_get_object (self->builder, "page1-filechooser")),
                                FALSE, FALSE);
        } else {

                 GcrCertificateChooserPkcs11 *data = gcr_certificate_chooser_pkcs11_new (slot, "page1");
                 data->builder = self->builder;
                 gtk_paned_add2(GTK_PANED(gtk_builder_get_object(self->builder, "page1-box")), GTK_WIDGET(data));
                 gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(self->builder, "page1-next-button")), FALSE);
                 gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(self->builder, "page1-box")));
        }

}

static void
on_page1_tree_view_selection_changed (GtkTreeSelection *selection,
                                gpointer data)
{

        GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG(data);
        gtk_tree_selection_selected_foreach (selection, on_page1_tree_node_select, self);
}

static void
on_page2_tree_node_select (GtkTreeModel *model,
                     GtkTreePath *path,
                     GtkTreeIter *iter,
                     gpointer user_data)
{

        GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG(user_data);
        GckSlot *slot;
        gchar *label;
        GtkWidget *previous_child;
        GcrCertificateChooserPkcs11 *token;

        gtk_tree_model_get (model, iter, ROW_TYPE, &label, COLUMN_SLOT, &slot, -1);
        gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(
                           self->builder, "key-label")),
                           no_key);
       
        previous_child =  gtk_paned_get_child2(GTK_PANED(gtk_builder_get_object(
                                               self->builder, "page2-box")));

        if (previous_child != NULL) {

                 if (!GTK_IS_FILE_CHOOSER (previous_child)) {

                          
                          token = GCR_CERTIFICATE_CHOOSER_PKCS11 (previous_child);
                          gck_session_logout (token->session,
                                              NULL,
                                              NULL);
                          gtk_widget_destroy (previous_child);
                 } else {
                 
                          gtk_container_remove (GTK_CONTAINER (gtk_builder_get_object(
                                                self->builder, "page2-box")),
                                                previous_child);
                 }
        }

        if (!slot) {
        
                gtk_paned_pack2(GTK_PANED(gtk_builder_get_object(self->builder, "page2-box")), 
                                GTK_WIDGET (gtk_builder_get_object (self->builder, "page2-filechooser")),
                                FALSE, FALSE);
        } else {

                 GcrCertificateChooserPkcs11 *data = gcr_certificate_chooser_pkcs11_new (slot, "page2");
                 data->builder = self->builder;
                 gtk_paned_add2(GTK_PANED(gtk_builder_get_object(self->builder, "page2-box")), GTK_WIDGET(data));
                 gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(self->builder, "page2-box")));
        }

}

static void
on_page2_tree_view_selection_changed (GtkTreeSelection *selection,
                                gpointer data)
{

        GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG(data);
        gtk_tree_selection_selected_foreach (selection, on_page2_tree_node_select, self);
}

static void 
gcr_certificate_chooser_sidebar_constructed (GObject *obj)
{
        GcrCertificateChooserSidebar *self = GCR_CERTIFICATE_CHOOSER_SIDEBAR(obj);
        GtkTreeViewColumn *col;
        GtkCellRenderer *cell;

        G_OBJECT_CLASS (gcr_certificate_chooser_sidebar_parent_class)->constructed (obj);
        //gtk_widget_size_request (GTK_WIDGET (self), 250, -1);
        self->tree_view = gtk_tree_view_new();
        col = gtk_tree_view_column_new ();

        cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, cell, TRUE);
	g_object_set (G_OBJECT (cell), "editable", FALSE, NULL);
	gtk_tree_view_column_set_cell_data_func (col, cell,
	                                         get_token_label,
	                                         self, NULL);

	g_object_set (cell,
	              "ellipsize", PANGO_ELLIPSIZE_END,
	              "ellipsize-set", TRUE,
	              NULL);

        gtk_tree_view_append_column (GTK_TREE_VIEW(self->tree_view), col);
        gtk_tree_view_column_set_max_width (GTK_TREE_VIEW_COLUMN (col), 12);
        gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->tree_view));
        gtk_widget_show (GTK_WIDGET (self->tree_view));
}
	            	            
static void
gcr_certificate_chooser_sidebar_class_init(GcrCertificateChooserSidebarClass *klass)
{

        GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
        gobject_class->constructed = gcr_certificate_chooser_sidebar_constructed;

}

GcrCertificateChooserSidebar *
gcr_certificate_chooser_sidebar_new ()
{

        GcrCertificateChooserSidebar *sidebar;
        sidebar = g_object_new(GCR_TYPE_CERTIFICATE_CHOOSER_SIDEBAR,
                            NULL);
        return g_object_ref_sink (sidebar);

}
        
static void
on_page3_choose_again_button_clicked(GtkWidget *widget,
                                 gpointer  *data)
{
       
       GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (data);
       printf ("asdfasdf");
       gtk_window_set_title(GTK_WINDOW(self), "Choose Certificate");

       gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(
                                   self->builder, "content-area")),
                                   GTK_WIDGET(gtk_builder_get_object(
                                   self->builder, "page1-box")));
       self->is_certificate_choosen = FALSE;
       self->is_key_choosen = FALSE;
       self->key_uri = NULL;
       self->certificate_uri = NULL;
       gtk_tree_view_set_cursor (GTK_TREE_VIEW (self->page1_sidebar->tree_view), gtk_tree_path_new_from_string("0"), NULL, FALSE);
       gtk_tree_view_set_cursor (GTK_TREE_VIEW (self->page2_sidebar->tree_view), gtk_tree_path_new_from_string("0"), NULL, FALSE);
       gtk_widget_set_visible (GTK_WIDGET (gtk_builder_get_object (self->builder, "page1-next-button")), TRUE);
       gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (self->builder, "page1-next-button")), FALSE);       
       gtk_widget_set_visible (GTK_WIDGET (gtk_builder_get_object (self->builder, "page2-next-button")), FALSE);
       gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (self->builder, "page2-next-button")), FALSE);       
       gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (self->builder, "key-label")), no_key);

}

static void 
on_page2_next_button_clicked(GtkWidget *widget,
                             gpointer *data)
{
      
       GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (data);
       gtk_window_set_title(GTK_WINDOW(self), "Confirm");

       gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(
                                   self->builder, "content-area")),
                                   GTK_WIDGET(gtk_builder_get_object(
                                   self->builder, "page3-box")));

       gtk_widget_set_visible (GTK_WIDGET (gtk_builder_get_object (self->builder, "page2-next-button")), FALSE);
       printf("The key uri is %s\n",self->key_uri);
       
}
               
static void
on_next_button_clicked(GtkWidget *widget, gpointer *data)
{
       GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (data);
       GtkFileChooserWidget *chooser;
       gchar *fname;
       g_free(self->certificate_uri);
       self->is_certificate_choosen = TRUE;
       gtk_window_set_title(GTK_WINDOW(self), "Choose Key");

       chooser = GTK_FILE_CHOOSER_WIDGET(gtk_builder_get_object(
                                  self->builder, "page1-filechooser"));

       fname = gtk_file_chooser_get_preview_filename(GTK_FILE_CHOOSER(chooser));
       self->certificate_uri = fname;

       gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(
                                   self->builder, "content-area")),
                                   GTK_WIDGET(gtk_builder_get_object(
                                   self->builder, "page2-box")));

       gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(
                              self->builder, "page1-next-button")),FALSE);

       gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(
                              self->builder, "page2-next-button")),TRUE);

       if (self->is_key_choosen) {

                 self->key_uri = fname;
                 gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(
                                               gtk_builder_get_object(
                                               self->builder, "page2-filechooser")),
                                               fname);
                  gtk_button_clicked(GTK_BUTTON(gtk_builder_get_object(
                                     self->builder, "page2-next-button")));
       }

       printf("next-button %s\n", fname);
}

static void
on_default_button_clicked(GtkWidget *widget, gpointer *data)
{
        GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (data);
        GtkFileChooserWidget *chooser;

        if (!self->is_certificate_choosen) {        
	         chooser = GTK_FILE_CHOOSER_WIDGET(gtk_builder_get_object(
                                                   self->builder, 
                                                   "page1-filechooser"));
        } else {
	         chooser = GTK_FILE_CHOOSER_WIDGET(gtk_builder_get_object(
                                                   self->builder, 
                                                   "page2-filechooser"));
        }

        gchar *fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (chooser));
        printf("default-button %s\n", fname);

        if (g_file_test(fname, G_FILE_TEST_IS_DIR)) {
	        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (chooser), fname);
	        g_free(fname);
        } else {
	        /* This is used as a flag to the preview/added signals that this was *set* */
                if(!self->is_certificate_choosen)
	            self->certificate_uri = fname;
                else
	            self->key_uri = fname;
	        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (chooser), fname);
        }
}

static void
on_parser_parsed_item(GcrParser *parser,
                      gpointer  *data)
{        GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (data); 
         GckAttributes *attributes;
         char *filename;
         gulong class;
         gchar *markup;
         printf ("in the parsed function/n");
         attributes = gcr_parser_get_parsed_attributes(parser);
     
         if (!self->is_certificate_choosen) {
                 filename = gtk_file_chooser_get_preview_filename(
                                    GTK_FILE_CHOOSER(gtk_builder_get_object(
                                    self->builder, "page1-filechooser")));
             
                 if (self->certificate_uri && g_strcmp0(self->certificate_uri, filename) != 0) {

                        g_free(self->certificate_uri);
		        self->certificate_uri = NULL;
	         }
        
                if (gck_attributes_find_ulong (attributes, CKA_CLASS, &class) && class == CKO_CERTIFICATE) {
		// XXX: Get subject, issuer and expiry and use gtk_label_set_markup to show them nicely
		//gtk_label_set_text(GTK_LABEL(self->cert_label), gcr_parser_get_parsed_label(parser));
                       markup = g_markup_printf_escaped ("%s", gcr_parser_get_parsed_label(parser));
                       gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(
                                   self->builder, "certificate-label")),
                                   markup);

                       gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(
                                                           self->builder,
                                                           "page1-next-button")), TRUE);

	               if (self->certificate_uri) {
	                          gtk_button_clicked(GTK_BUTTON(gtk_builder_get_object(
                                                     self->builder,
                                                     "page1-next-button")));
                       } else 
                             printf("not set\n");
                 }
        }

        if (gck_attributes_find_ulong (attributes, CKA_CLASS, &class) && class == CKO_PRIVATE_KEY){
            self->is_key_choosen = TRUE;
	    if (self->key_uri) {
	               gtk_button_clicked(GTK_BUTTON(gtk_builder_get_object(
                                          self->builder,
                                          "page2-next-button")));
             } else 
                    printf("not set\n");
            
            gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(
                               self->builder, "key-label")),
                               "Key selected");

            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(
                                     self->builder, "page2-next-button")), TRUE);
        }

     
}        

static void
on_file_activated(GtkWidget *widget, gpointer *data)
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(widget);

	/* The user might have hit Ctrl-L and typed in a filename, and this
	   happens when they press Enter. It g_object_newalso happens when they double
	   click a file in the browser. */
	gchar *fname = gtk_file_chooser_get_filename(chooser);
	gtk_file_chooser_set_filename(chooser, fname);
	printf("fname chosen: %s\n", fname);
	g_free(fname);

	/* if Next button activated, then behave as if it's pressed */
}

static gboolean
on_unlock_renderer_clicked(GtkEntry *entry, 
                           gpointer user_data)
{
	GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (user_data);
        GError *error = NULL;
        GtkFileChooser *chooser;

        if (!self->is_certificate_choosen) {
	         chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(
                                            self->builder, 
                                            "page1-filechooser"));
        } else {
	         chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(
                                            self->builder, 
                                            "page2-filechooser"));
        }

        gcr_parser_add_password(self->parser, gtk_entry_get_text (GTK_ENTRY (entry)));
        if (gcr_parser_parse_bytes (self->parser, self->data, &error)) {

                gtk_widget_destroy (gtk_file_chooser_get_preview_widget (chooser));
                gtk_file_chooser_set_preview_widget_active(GTK_FILE_CHOOSER(chooser), FALSE);
                g_object_unref (entry);

        }
        return TRUE;

      
}

static gboolean 
on_parser_authenticate_for_data(GcrParser *parser,
                         gint count,
                         gpointer *data_main)
{
     
	GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (data_main);
        GtkFileChooserWidget *chooser;
        GtkWidget *box, *entry;

        if (!self->is_certificate_choosen) {
	         chooser = GTK_FILE_CHOOSER_WIDGET(gtk_builder_get_object(
                                                   self->builder, 
                                                   "page1-filechooser"));
        } else {
	         chooser = GTK_FILE_CHOOSER_WIDGET(gtk_builder_get_object(
                                                   self->builder, 
                                                   "page2-filechooser"));
        }
        gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER(chooser), TRUE);
        box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
        entry = gtk_entry_new ();
        gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
        gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (gtk_label_new ("Enter Pin")));
        gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (entry));
        gtk_widget_show_all (GTK_WIDGET (box));
       
        gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(chooser), GTK_WIDGET (box));
        self->data = gcr_parser_get_parsed_bytes (parser);
        g_signal_connect(entry, "activate", G_CALLBACK(on_unlock_renderer_clicked), self);
        self->password_wrong_count += 1;

        return TRUE;
}

static void 
on_page2_update_preview(GtkWidget *widget, 
                        gpointer *user_data)
{

	GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (user_data);
	
        GError *err = NULL;
        guchar *data;
        gsize n_data;
        GBytes *bytes;
        self->password_wrong_count = 0;
        self->is_key_choosen = FALSE;

	GtkFileChooser *chooser = GTK_FILE_CHOOSER(widget);

	char *filename = gtk_file_chooser_get_preview_filename(chooser);

        gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(
                           self->builder, "key-label")),
                           no_key);
    
	gtk_file_chooser_set_preview_widget_active(chooser, FALSE);

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(
                                 self->builder, "page2-next-button")),
                                 FALSE);

	if (!filename || g_file_test(filename, G_FILE_TEST_IS_DIR))
		return;
        
        if(!g_file_get_contents (filename, (gchar**)&data, &n_data, NULL))
            printf("couldn't read file");

        
        bytes = g_bytes_new_take(data, n_data); 
        if (!gcr_parser_parse_bytes (self->parser, bytes, &err))
            printf("couldn't parse data: %s", err->message);

	printf("Preview %s\n", filename);
	g_free(filename);

}

static void
on_page1_update_preview(GtkWidget *widget, gpointer *user_data)
{
	GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (user_data);
	
        GError *err = NULL;
        guchar *data;
        gsize n_data;
        GBytes *bytes;
        self->password_wrong_count = 0;
        self->is_key_choosen = FALSE;
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(widget);

	char *filename = gtk_file_chooser_get_preview_filename(chooser);
        gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(
                           self->builder, "key-label")),
                           no_key);
        
        gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(
                           self->builder, "certificate-label")),
                           no_certificate);
    
	gtk_file_chooser_set_preview_widget_active(chooser, FALSE);

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(
                                 self->builder, "page1-next-button")),
                                  FALSE);
	if (!filename || g_file_test(filename, G_FILE_TEST_IS_DIR))
		return;
        
        if(!g_file_get_contents (filename, (gchar**)&data, &n_data, NULL))
            printf("couldn't read file");

        
        bytes = g_bytes_new_take(data, n_data); 
        if (!gcr_parser_parse_bytes (self->parser, bytes, &err))
            printf("couldn't parse data: %s", err->message);
       
     
	if (self->certificate_uri && g_strcmp0(self->certificate_uri,
                                               filename) != 0) {
		g_free(self->certificate_uri);
		self->certificate_uri = NULL;
	}

	printf("Preview %s\n", filename);
	g_free(filename);
}

static gboolean
is_token_usable (GcrCertificateChooserDialog *self,
                 GckSlot *slot,
                 GckTokenInfo *token)
{
        GList *l;

        if (!(token->flags & CKF_TOKEN_INITIALIZED)) {
                return FALSE;
        }
        if ((token->flags & CKF_LOGIN_REQUIRED) &&
            !(token->flags & CKF_USER_PIN_INITIALIZED)) {
                return FALSE;
        }

        for (l = self->blacklist; l != NULL; l = g_list_next (l)) {
                if (gck_slot_match (slot, l->data)) 
                        return FALSE;
        }

        return TRUE;
}

static void
on_initialized_registered (GObject *unused,
                           GAsyncResult *result,
                           gpointer user_data)
{
        GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (user_data);
        GList *slots, *s;
        GList *modules, *m;
        GtkTreeIter iter;
        GError *error = NULL;
        GckTokenInfo *token;
        self->slots = NULL;

        modules = gck_modules_initialize_registered_finish (result, &error);
        if (error != NULL) {
                 g_warning ("%s", error->message);
                 g_clear_error (&error);
        }

        for (m = modules; m != NULL; m = g_list_next (m)) {
                 slots = gck_module_get_slots (m->data, TRUE);
                 for (s = slots; s; s = g_list_next (s)) {
                          token = gck_slot_get_token_info (s->data);
                          if (token == NULL)
                                   continue;
                          if (is_token_usable (self, s->data, token)) {

                                   gtk_list_store_append (self->store, &iter);
                                   gtk_list_store_set(self->store, &iter, ROW_TYPE, pkcs11_token, COLUMN_SLOT, s->data, -1);
                                   self->slots = g_list_append (self->slots, s->data);
                          }
                          gck_token_info_free (token);
                 }

                 gck_list_unref_free (slots);
        }

        self->loaded = TRUE;
        gck_list_unref_free (modules);
        g_object_unref (self);
}
/*
static void 
on_token_load(GObject *obj, gpointer *data)
{
       
	GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (obj);
        GtkTreeIter iter;
        GcrCertificateChooserSidebar *sidebar;
        GList *l;
        GckTokenInfo *info;
        GcrCertificateChooserPkcs11 *pkcs11;
        for(l = self->slots; l != NULL; l = g_list_next(l)) {
               gtk_list_store_append(self->store, &iter);
               pkcs11 = (GcrCertificateChooserPkcs11 *)l->data;
               info = pkcs11->info;
               gtk_list_store_set(self->store, &iter,COLUMN_STRING, (gchar *)info->label, -1);
               gck_token_info_free (info);
               g_object_unref (pkcs11);
        }
        sidebar = gcr_certificate_chooser_sidebar_new();
        gtk_tree_view_set_model(GTK_TREE_VIEW(sidebar->tree_view),GTK_TREE_MODEL(self->store));
        gtk_container_add(GTK_CONTAINER(gtk_builder_get_object(self->builder, "page1-pkcs11")), GTK_WIDGET(sidebar));
        gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(self->builder, "page1-pkcs11")));        
       
}	

*/
static void
gcr_certificate_chooser_dialog_constructed (GObject *obj)
{
	GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (obj);
        GtkWidget *content;
        GtkTreeSelection *page1_selection, *page2_selection;
        GError *error = NULL;
        G_OBJECT_CLASS (gcr_certificate_chooser_dialog_parent_class)->constructed (obj);
        gck_modules_initialize_registered_async(NULL, on_initialized_registered,
                                                g_object_ref(self));

        if (!gtk_builder_add_from_file (self->builder, UIDIR"gcr-certificate-chooser-dialog.ui", &error)) {
                  g_warning ("couldn't load ui builder file: %s", error->message);
                  return;
          }
        printf ("%s\n", UIDIR);

	content = GTK_WIDGET(GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))));
 
        gtk_window_set_default_size (GTK_WINDOW (self), 1200, 800);

	self->hbox = GTK_WIDGET(gtk_builder_get_object(self->builder, "certificate-chooser-dialog"));

        gtk_box_pack_start (GTK_BOX(content), self->hbox, TRUE, TRUE, 0);

        gtk_image_set_from_gicon(GTK_IMAGE(gtk_builder_get_object(
                                 self->builder, "certificate-image")),
                                 g_themed_icon_new(GCR_ICON_CERTIFICATE),
                                 GTK_ICON_SIZE_DIALOG);
        gtk_image_set_from_gicon(GTK_IMAGE(gtk_builder_get_object(
                                 self->builder, "key-image")),
                                 g_themed_icon_new(GCR_ICON_KEY),
                                 GTK_ICON_SIZE_DIALOG);
        gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(
                           self->builder, "key-label")),
                           no_key);
        
        gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(
                           self->builder, "certificate-label")),
                           no_certificate);

        g_signal_connect(self->parser, "parsed",
                         G_CALLBACK(on_parser_parsed_item), self);

        g_signal_connect(self->parser, "authenticate",
                         G_CALLBACK(on_parser_authenticate_for_data), self);

       /* Page1 Construction */
        self->page1_sidebar = gcr_certificate_chooser_sidebar_new();
        page1_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(self->page1_sidebar->tree_view));
        gtk_tree_selection_set_mode (page1_selection, GTK_SELECTION_MULTIPLE);
        g_signal_connect (page1_selection, "changed", G_CALLBACK(on_page1_tree_view_selection_changed), self);
        gtk_tree_view_set_model(GTK_TREE_VIEW(self->page1_sidebar->tree_view),GTK_TREE_MODEL(self->store));
        gtk_paned_pack1(GTK_PANED(gtk_builder_get_object(self->builder, "page1-box")), GTK_WIDGET(self->page1_sidebar),TRUE, FALSE);
       // gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(self->builder, "page1-pkcs11")));

	GtkFileFilter *page1_filefilter = GTK_FILE_FILTER(gtk_builder_get_object(self->builder, "page1-filefilter"));
	gtk_file_filter_set_name (page1_filefilter,"X.509 Certificate Format");

        GtkFileChooserWidget *page1_filechooser = GTK_FILE_CHOOSER_WIDGET(gtk_builder_get_object(self->builder,
                                                    "page1-filechooser"));
	g_signal_connect(GTK_BUTTON(gtk_builder_get_object(
                         self->builder, "default-button")),
                         "clicked", G_CALLBACK(on_default_button_clicked),
                         self);

        gtk_container_add(GTK_CONTAINER(content),
                          GTK_WIDGET(gtk_builder_get_object(
                          self->builder, "default-button")));
	
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(page1_filechooser),page1_filefilter);

        gtk_widget_set_size_request (GTK_WIDGET (self->page1_viewer), 200, 200);
        gtk_widget_set_halign (GTK_WIDGET (self->page1_viewer), GTK_ALIGN_END);

	g_signal_connect(GTK_FILE_CHOOSER (page1_filechooser),
                         "update-preview", G_CALLBACK (
                          on_page1_update_preview), self);

	g_signal_connect(GTK_FILE_CHOOSER (page1_filechooser),
                         "file-activated", G_CALLBACK (
                          on_file_activated), self);

        g_signal_connect(GTK_WIDGET(gtk_builder_get_object(self->builder,
                         "page1-next-button")), "clicked",
                          G_CALLBACK(on_next_button_clicked), self);

	gtk_widget_grab_default(GTK_WIDGET(gtk_builder_get_object(
                                self->builder, "default-button")));

        /* Page2 Construction */
        self->page2_sidebar = gcr_certificate_chooser_sidebar_new();
        page2_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(self->page2_sidebar->tree_view));
        gtk_tree_selection_set_mode (page2_selection, GTK_SELECTION_MULTIPLE);
        g_signal_connect (page2_selection, "changed", G_CALLBACK(on_page2_tree_view_selection_changed), self);
        gtk_tree_view_set_model(GTK_TREE_VIEW(self->page2_sidebar->tree_view),GTK_TREE_MODEL(self->store));
        gtk_paned_pack1(GTK_PANED(gtk_builder_get_object(self->builder, "page2-box")), GTK_WIDGET(self->page2_sidebar), TRUE, FALSE);

	GtkFileFilter *page2_filefilter = GTK_FILE_FILTER(gtk_builder_get_object(self->builder, "page2-filefilter"));
	gtk_file_filter_set_name (page2_filefilter,"X.509 Key Format");
       
        GtkFileChooserWidget *page2_file_chooser = GTK_FILE_CHOOSER_WIDGET(gtk_builder_get_object(self->builder,
                                                    "page2-filechooser"));
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (page2_file_chooser),
                                     page2_filefilter); 
 
	g_signal_connect(GTK_FILE_CHOOSER (page2_file_chooser),
                         "update-preview", G_CALLBACK (
                         on_page2_update_preview), self);

	g_signal_connect(GTK_BUTTON (gtk_builder_get_object(
                         self->builder, "page2-next-button")), 
                         "clicked", G_CALLBACK (
                         on_page2_next_button_clicked),self);

	g_signal_connect(GTK_FILE_CHOOSER (page2_file_chooser),
                         "file-activated", G_CALLBACK (
                         on_file_activated), self);
       
        
	g_signal_connect(GTK_BUTTON (gtk_builder_get_object(
                         self->builder, "choose-again-button")),
                         "clicked", G_CALLBACK (
                         on_page3_choose_again_button_clicked), self);
       
	gtk_widget_show_all(GTK_WIDGET (self));

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(
                        self->builder, "default-button")), FALSE);

	gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(
                        self->builder, "page2-next-button")), FALSE);
	gtk_window_set_modal (GTK_WINDOW (self), TRUE);
}

static void
gcr_certificate_chooser_dialog_init (GcrCertificateChooserDialog *self)
{
        GError *error = NULL;
        GckUriData *uri;
        guint i;
        self->loaded = FALSE;
        GtkTreeIter iter;
        self->blacklist = NULL;

        for (i = 0; token_blacklist[i] != NULL; i++) {
                 uri = gck_uri_parse (token_blacklist[i], GCK_URI_FOR_TOKEN | GCK_URI_FOR_MODULE, &error);
                 if (uri == NULL) {
                          g_warning ("couldn't parse pkcs11 blacklist uri: %s", error->message);
                          g_clear_error (&error);
                 }
                 self->blacklist = g_list_prepend (self->blacklist, uri);
        }



        self->builder = gtk_builder_new();       
        self->store = gtk_list_store_new (N_COLUMNS,
                                          G_TYPE_STRING,
                                          GCK_TYPE_SLOT);

        gtk_list_store_append (self->store, &iter);
        gtk_list_store_set(self->store, &iter, ROW_TYPE, "file-system", COLUMN_SLOT, NULL, -1);
        self->parser = gcr_parser_new();
        self->page1_viewer = gcr_viewer_new();
        self->page2_viewer = gcr_viewer_new();
        self->is_certificate_choosen = FALSE;
        self->password_wrong_count = 0;
        gtk_window_set_title(GTK_WINDOW(self), "Choose Certificate");

}

static void
gcr_certificate_chooser_dialog_finalize (GObject *obj)
{
	//GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (obj);

	G_OBJECT_CLASS (gcr_certificate_chooser_dialog_parent_class)->finalize (obj);
}

static void
gcr_certificate_chooser_dialog_class_init (GcrCertificateChooserDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->constructed = gcr_certificate_chooser_dialog_constructed;
	gobject_class->finalize = gcr_certificate_chooser_dialog_finalize;
        certificate_chooser_signals[TOKEN_ADDED] =
                           g_signal_new("token-added",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                        G_STRUCT_OFFSET (GcrCertificateChooserDialogClass, token_added),
                                        NULL, NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, 0);

}

/**
 * gcr_certificate_chooser_dialog_new:
 * @parent: the parent window
 *
 * Create a new certxificate chooser dialog.
 *
 * Returns: (transfer full): A new #GcrCertificateChooserDialog object
 */
GcrCertificateChooserDialog *
gcr_certificate_chooser_dialog_new (GtkWindow *parent)
{
	GcrCertificateChooserDialog *dialog;

	g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);

	dialog = g_object_new (GCR_TYPE_CERTIFICATE_CHOOSER_DIALOG,
	                       NULL);

	return g_object_ref_sink (dialog);
}


gboolean
gcr_certificate_chooser_dialog_run (GcrCertificateChooserDialog *self)
{
	gboolean ret = FALSE;

	g_return_val_if_fail (GCR_IS_CERTIFICATE_CHOOSER_DIALOG (self), FALSE);

	if (gtk_dialog_run (GTK_DIALOG (self)) == GTK_RESPONSE_OK) {
		ret = TRUE;
	}

	gtk_widget_hide (GTK_WIDGET (self));

	return ret;
}

void
gcr_certificate_chooser_dialog_run_async (GcrCertificateChooserDialog *self,
                                     GCancellable *cancellable,
                                     GAsyncReadyCallback callback,
                                     gpointer user_data)
{
	g_return_if_fail (GCR_IS_CERTIFICATE_CHOOSER_DIALOG (self));

	_gcr_dialog_util_run_async (GTK_DIALOG (self), cancellable, callback, user_data);
}

gboolean
gcr_certificate_chooser_dialog_run_finish (GcrCertificateChooserDialog *self,
                                      GAsyncResult *result)
{
	gint response;

	g_return_val_if_fail (GCR_IS_CERTIFICATE_CHOOSER_DIALOG (self), FALSE);

	response = _gcr_dialog_util_run_finish (GTK_DIALOG (self), result);

	gtk_widget_hide (GTK_WIDGET (self));

	return (response == GTK_RESPONSE_OK) ? TRUE : FALSE;
}

