/*
 * Copyright (C) 2007 OpenedHand Ltd
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <gtk/gtk.h>
#include <moko-stock.h>

#include "openmoko-contacts.h"
#include "hito-group-store.h"
#include "hito-group.h"

static void
groups_name_cell_data_func (GtkTreeViewColumn *col, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  HitoGroup *group;
  gtk_tree_model_get (model, iter, 0, &group, -1);
  g_object_set (G_OBJECT (cell), "text", hito_group_get_name (group), NULL);
  g_object_unref (group);
}

static void
groups_toggle_cell_data_func (GtkTreeViewColumn *col, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, ContactsData *data)
{
  HitoGroup *group;
  EContact *contact = NULL;

  contact = g_object_get_data (G_OBJECT (data->groups), "contact");
  if (!contact)
    return;
  gtk_tree_model_get (model, iter, 0, &group, -1);
  g_object_set (G_OBJECT (cell), "active", hito_group_includes_contact (group, contact), NULL);
  g_object_unref (group);
}


void
create_contacts_groups_page (ContactsData *data)
{
  GtkWidget *box;
  GtkTreeViewColumn *col;
  GtkCellRenderer *cell;

  box = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (data->notebook), box, gtk_image_new_from_stock (MOKO_STOCK_CONTACT_GROUPS, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_container_child_set (GTK_CONTAINER (data->notebook), box, "tab-expand", TRUE, "tab-fill", TRUE, NULL);

  data->groups_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (box), data->groups_label, FALSE, FALSE, 0);

  data->groups_liststore = hito_group_store_new ();
  hito_group_store_set_view (HITO_GROUP_STORE (data->groups_liststore), data->view);

  data->groups = gtk_tree_view_new_with_model (data->groups_liststore);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->groups), FALSE);
  gtk_box_pack_start (GTK_BOX (box), data->groups, TRUE, TRUE, 0);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->groups), col);

  cell = gtk_cell_renderer_toggle_new ();
  gtk_tree_view_column_pack_start (col, cell, FALSE);
  gtk_tree_view_column_set_cell_data_func (col, cell, (GtkTreeCellDataFunc) groups_toggle_cell_data_func, data, NULL);


  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, cell, TRUE);
  gtk_tree_view_column_set_cell_data_func (col, cell, groups_name_cell_data_func, NULL, NULL);
}

void
free_contacts_groups_page (ContactsData *data)
{
}

void
contacts_groups_page_set_contact (ContactsData *data, EContact *contact)
{
  const gchar *s;

  /* set the title of the page */
  s = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
  if (s)
  {
    gchar *markup;
    markup = g_markup_printf_escaped ("<b>%s</b>", s);
    gtk_label_set_markup (GTK_LABEL (data->groups_label), markup);
    g_free (markup);
  }
  else
    gtk_label_set_markup (GTK_LABEL (data->groups_label), "<b>Communication History</b>");

  g_object_set_data (G_OBJECT (data->groups), "contact", contact);

}


