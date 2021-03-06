/* 
 *  Contacts - A small libebook-based address book.
 *
 *  Authored By Chris Lord <chris@linux.intel.com>
 *
 *  Copyright (c) 2005, 2009 Intel Corporation
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
#include <libebook/e-book.h>
#include "contacts-defs.h"

void contacts_search_changed_cb (GtkWidget *search_entry, ContactsData *data);

void contacts_chooser_entry_changed_cb (GtkWidget *entry, ContactsData *data);
void contacts_chooser_add_cb (GtkWidget *button, ContactsData *data);

void contacts_chooser_toggle_cb (GtkCellRendererToggle * cell, gchar * path_string, gpointer user_data);

void contacts_selection_cb (GtkTreeSelection * selection, ContactsData *data);

void contacts_new_cb (GtkWidget *source, ContactsData *data);

void contacts_edit_cb (GtkWidget *source, ContactsData *data);

void contacts_treeview_edit_cb (GtkTreeView *treeview, GtkTreePath *arg1,
				GtkTreeViewColumn *arg2, ContactsData *data);
	
void contacts_delete_cb (GtkWidget *source, ContactsData *data);

int contacts_import (ContactsData *data, const gchar *filename,
		      gboolean do_confirm);

void contacts_import_cb (GtkWidget *source, ContactsData *data);

void contacts_export (ContactsData *data, const gchar *filename);

void contacts_export_cb (GtkWidget *source, ContactsData *data);

void contacts_edit_menu_activate_cb (GtkWidget *widget, ContactsData *data);

void contacts_copy_cb (GtkWindow *main_window);

void contacts_cut_cb (GtkWindow *main_window);

void contacts_paste_cb (GtkWindow *main_window);

void contacts_about_cb (GtkWidget *dialog);

gboolean contacts_treeview_search_cb (GtkWidget *search_entry,
				      GdkEventKey *event,
				      GtkTreeView *treeview);

gboolean contacts_is_row_visible_cb (GtkTreeModel * model, GtkTreeIter * iter,
				     GHashTable *contacts_table);

gint contacts_sort_treeview_cb (GtkTreeModel * model, GtkTreeIter * a,
				GtkTreeIter * b, gpointer user_data);

gboolean contacts_main_window_delete_event_cb (GtkWidget *main_window, gpointer data);
