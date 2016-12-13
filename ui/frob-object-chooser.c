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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gcr/gcr.h"

#include "ui/gcr-object-chooser-dialog.h"

#include <gtk/gtk.h>

int
main(int argc, char *argv[])
{
	GtkWidget *dialog;

	gtk_init (&argc, &argv);
	g_set_prgname ("frob-object-chooser");

	dialog = gcr_object_chooser_dialog_new ("Choose a certificate", NULL,
	                                        GTK_DIALOG_USE_HEADER_BAR,
	                                        "_Cancel", GTK_RESPONSE_CANCEL,
	                                        "_Select", GTK_RESPONSE_ACCEPT,
	                                        NULL, NULL);
	if (argc > 1) {
		gcr_object_chooser_set_uri (GCR_OBJECT_CHOOSER (dialog), argv[1]);
	}

	switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
	case GTK_RESPONSE_ACCEPT:
		g_printerr ("selected: %s\n", gcr_object_chooser_get_uri (GCR_OBJECT_CHOOSER (dialog)));
		break;
	default:
		g_printerr ("cancelled\n");
	}
	gtk_widget_destroy (dialog);

	return 0;
}
