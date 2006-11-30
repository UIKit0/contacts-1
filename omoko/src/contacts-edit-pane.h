/* 
 *  Contacts - A small libebook-based address book.
 *
 *  Authored By Chris Lord <chris@o-hand.com>
 *
 *  Copyright (c) 2005 OpenedHand Ltd - http://o-hand.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <glib.h>
#include <gtk/gtk.h>
#include "contacts-defs.h"

void contacts_edit_pane_show (ContactsData *data, gboolean new);

void contacts_edit_set_focus_cb (GtkWindow *window, GtkWidget *widget,
				 gpointer user_data);

void contacts_remove_field_cb (GtkWidget *button, gpointer data);

GtkWidget *contacts_label_widget_new (EContact *contact, EVCardAttribute *attr,
				      const gchar *name, gboolean multi_line);

void contacts_remove_edit_widgets_cb (GtkWidget *widget, gpointer data);

void contacts_append_to_edit_table (GtkTable *table, GtkWidget *label,
				    GtkWidget *edit, gboolean do_focus);
