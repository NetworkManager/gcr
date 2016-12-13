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

#ifndef __GCR_TOKEN_LOGIN_DIALOG_H__
#define __GCR_TOKEN_LOGIN_DIALOG_H__

#include <gtk/gtk.h>
#include <gck/gck.h>

typedef struct _GcrTokenLoginDialogPrivate GcrTokenLoginDialogPrivate;

struct _GcrTokenLoginDialog
{
        GtkDialog parent_instance;
        GcrTokenLoginDialogPrivate *priv;
};

#define GCR_TYPE_TOKEN_LOGIN_DIALOG gcr_token_login_dialog_get_type ()
G_DECLARE_FINAL_TYPE (GcrTokenLoginDialog, gcr_token_login_dialog, GCR, TOKEN_LOGIN_DIALOG, GtkDialog)

GtkWidget *gcr_token_login_dialog_new (GckSlot *slot);

const guchar *gcr_token_login_dialog_get_pin_value (GcrTokenLoginDialog *self);

gulong gcr_token_login_dialog_get_pin_length (GcrTokenLoginDialog *self);

gboolean gcr_token_login_dialog_get_remember_pin (GcrTokenLoginDialog *self);

#endif /* __GCR_TOKEN_LOGIN_DIALOG_H__ */
