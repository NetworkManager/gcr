/*
 * Copyright (C) 2017 Red Hat
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

#include "config.h"

#include "ui/gcr-ui.h"

#include "ui/gcr-certificate-chooser-button.h"

#include <gtk/gtk.h>

int
main (int argc, char *argv[])
{
	GtkDialog *dialog;
	GtkWidget *button;

	gtk_init (&argc, &argv);

	dialog = GTK_DIALOG (gtk_dialog_new ());
	g_object_ref_sink (dialog);

	button = GTK_WIDGET (gcr_certificate_chooser_button_new (GTK_WINDOW (dialog)));
	gtk_widget_show_all (button);

	gtk_widget_set_halign (button, 0.5);
	gtk_widget_set_valign (button, 0.5);

	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (dialog)), button);

	gtk_window_set_default_size (GTK_WINDOW (dialog), 200, 300);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 20);

	gtk_dialog_run (dialog);

	gtk_widget_destroy (GTK_WIDGET (dialog));
	g_object_unref (dialog);

	return 0;
}
