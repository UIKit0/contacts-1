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
#include "hito-vcard-util.h"

static void toggle_toggled_cb (GtkCellRendererToggle *renderer, gchar *path, ContactsData *data);
static void commit_contact (ContactsData *data);
static void add_groups_clicked_cb (GtkWidget *button, ContactsData *Data);


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
  GtkWidget *box, *w;
  GtkTreeViewColumn *col;
  GtkCellRenderer *cell;

  box = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (data->notebook), box, gtk_image_new_from_stock (MOKO_STOCK_CONTACT_GROUPS, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_container_child_set (GTK_CONTAINER (data->notebook), box, "tab-expand", TRUE, "tab-fill", TRUE, NULL);
  g_signal_connect_swapped (box, "unmap", G_CALLBACK (commit_contact), data);

  data->groups_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (box), data->groups_label, FALSE, FALSE, 0);

  data->groups_liststore = hito_group_store_new ();
  hito_group_store_set_view (HITO_GROUP_STORE (data->groups_liststore), data->view);
  g_object_set_data (G_OBJECT (data->groups_liststore), "dirty", GINT_TO_POINTER (FALSE));

  data->groups = gtk_tree_view_new_with_model (data->groups_liststore);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->groups), FALSE);
  gtk_box_pack_start (GTK_BOX (box), data->groups, FALSE, FALSE, 0);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->groups), col);

  cell = gtk_cell_renderer_toggle_new ();
  gtk_tree_view_column_pack_start (col, cell, FALSE);
  g_signal_connect (cell, "toggled", G_CALLBACK (toggle_toggled_cb), data);
  gtk_tree_view_column_set_cell_data_func (col, cell, (GtkTreeCellDataFunc) groups_toggle_cell_data_func, data, NULL);


  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, cell, TRUE);
  gtk_tree_view_column_set_cell_data_func (col, cell, groups_name_cell_data_func, NULL, NULL);


  w = gtk_button_new_with_label ("Add Group");
  gtk_widget_set_name (w, "moko-contacts-add-detail-button");
  g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (add_groups_clicked_cb), data);
  gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);

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


static void
toggle_toggled_cb (GtkCellRendererToggle *renderer, gchar *path, ContactsData *data)
{
  HitoGroup *group;
  gboolean toggle;
  EContact *contact;
  GtkTreeIter iter;
  EVCardAttribute *attr;

  g_object_get (G_OBJECT (renderer), "active", &toggle, NULL);

  contact = g_object_get_data (G_OBJECT (data->groups), "contact");
  if (!contact)
    return;
  gtk_tree_model_get_iter_from_string (data->groups_liststore, &iter, path);
  gtk_tree_model_get (data->groups_liststore, &iter, 0, &group, -1);
  
  attr = e_vcard_get_attribute (E_VCARD (contact), EVC_CATEGORIES);
  if (!attr)
  {
    attr = e_vcard_attribute_new (NULL, EVC_CATEGORIES);
    e_vcard_add_attribute (E_VCARD (contact), attr);
  }

  if (hito_group_includes_contact (group, contact))
  {
    /* remove group from contact */
    e_vcard_attribute_remove_value (attr, hito_group_get_name (group));
  }
  else
  {
    /* add group to contact */
    e_vcard_attribute_add_value (attr, hito_group_get_name (group));
  }

  g_object_set_data (G_OBJECT (data->groups_liststore), "dirty", GINT_TO_POINTER (TRUE));

}

static void
commit_contact (ContactsData *data)
{
  EContact *old_contact;
  GtkTreeModel *liststore;
  gboolean dirty;

  /* Clear up any loose ends */

  liststore = data->groups_liststore;
  dirty = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (liststore), "dirty"));

  if (dirty)
  {
    const gchar *uid;
    old_contact = g_object_get_data (G_OBJECT (data->groups), "contact");
    uid = e_contact_get_const (old_contact, E_CONTACT_UID);

    hito_vcard_strip_empty_attributes (E_VCARD (old_contact));

    /* if the contact doesn't have a uid then it is a newly created contact */
    if (!uid)
    {
      e_book_async_add_contact (data->book, old_contact, NULL, NULL);
      g_object_unref (old_contact);
    }
    else
    {
      /* TODO: error checking on failure */
      e_book_async_commit_contact (data->book, old_contact, NULL, NULL);
    }
  }

  g_object_set_data (G_OBJECT (data->groups_liststore), "dirty", GINT_TO_POINTER (FALSE));

}


static void
add_groups_clicked_cb (GtkWidget *button, ContactsData *data)
{
  HitoGroup *new_group;
  gchar *new_name;
  GtkDialog *d; /* temporary dialog until group rename is implemented */
  GtkWidget *w;

  d = gtk_dialog_new_with_buttons ("New Group", data->window,
      GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

  w = gtk_entry_new ();
  gtk_container_add (d->vbox, w);
  gtk_widget_show (w);

  if (gtk_dialog_run (d) == GTK_RESPONSE_OK)
  {
    new_name = gtk_entry_get_text (GTK_ENTRY (w));

    new_group = hito_category_group_new (new_name);

    hito_group_store_add_group (data->groups_liststore, new_group);

    g_object_unref (new_group);

  }
  gtk_widget_destroy (d);
}
