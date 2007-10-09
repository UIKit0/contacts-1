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
#include <moko-finger-scroll.h>

#include "openmoko-contacts.h"
#include "hito-group-store.h"
#include "hito-category-group.h"
#include "hito-group.h"
#include "hito-vcard-util.h"
#include "koto-cell-renderer-pixbuf.h"


typedef struct {
  ContactsData *contacts_data;
  const gchar *name;
} DeleteData;

typedef struct {
  ContactsData *contacts_data;
  const gchar *old_name;
  gchar *new_name;
} RenameData;

static void toggle_toggled_cb (GtkCellRendererToggle *renderer, gchar *path, ContactsData *data);
static void add_groups_clicked_cb (GtkWidget *button, ContactsData *Data);
static void delete_renderer_activated_cb (KotoCellRendererPixbuf *cell, const char *path, ContactsData *data);


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

  contact = data->contact;
  if (!contact)
    return;
  gtk_tree_model_get (model, iter, 0, &group, -1);
  g_object_set (G_OBJECT (cell), "active", hito_group_includes_contact (group, contact), NULL);
  g_object_unref (group);
}

gboolean
groups_rename_contact_group (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, RenameData *data)
{
  EContact *contact;
  EVCardAttribute *attr;

  gtk_tree_model_get (model, iter, COLUMN_CONTACT, &contact, -1);

  attr = e_vcard_get_attribute (E_VCARD (contact), EVC_CATEGORIES);
  if (attr && hito_vcard_attribute_has_value (attr, data->old_name))
  {
    e_vcard_attribute_remove_value (attr, data->old_name);

    if (!hito_vcard_attribute_has_value (attr, data->new_name))
      e_vcard_attribute_add_value (attr, data->new_name);

    e_book_async_commit_contact (data->contacts_data->book, contact, NULL, NULL);
  }

  /* continue gtk_tree_model_foreach */
  return FALSE;
}

void
groups_name_edited_cb (GtkCellRendererText *renderer, gchar *path, gchar *new_text, ContactsData *data)
{
  RenameData r_data;
  HitoGroup *group;
  GtkTreeIter iter;

  r_data.contacts_data = data;
  r_data.new_name = new_text;

  gtk_tree_model_get_iter_from_string (data->groups_liststore, &iter, path);
  gtk_tree_model_get (data->groups_liststore, &iter, 0, &group, -1);
  r_data.old_name = hito_group_get_name (group);
  g_object_unref (group);


  gtk_tree_model_foreach (data->contacts_store,
		  (GtkTreeModelForeachFunc) groups_rename_contact_group, &r_data);

  /* hito doesn't yet remove the group when it is empty */
  hito_group_store_remove_group (HITO_GROUP_STORE (data->groups_liststore), group);
}

void
create_contacts_groups_page (ContactsData *data)
{
  GtkWidget *w, *sw, *vbox;
  GtkTreeViewColumn *col;
  GtkCellRenderer *cell;

  data->groups_box = gtk_vbox_new (FALSE, 0);
  contacts_notebook_add_page_with_icon (data->notebook, data->groups_box, MOKO_STOCK_CONTACT_GROUPS);

  data->groups_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (data->groups_box), data->groups_label, FALSE, FALSE, 0);

  data->groups_liststore = hito_group_store_new ();
  hito_group_store_set_view (HITO_GROUP_STORE (data->groups_liststore), data->view);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), PADDING);
  gtk_box_pack_start (GTK_BOX (data->groups_box), vbox, TRUE, TRUE, 0);

  w = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (w), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), w, TRUE, TRUE, 0);

  sw = moko_finger_scroll_new ();
  gtk_container_add (GTK_CONTAINER (w), sw);
  

  data->groups = gtk_tree_view_new_with_model (data->groups_liststore);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->groups), FALSE);
  gtk_container_add (GTK_CONTAINER (sw), data->groups);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->groups), col);

  cell = gtk_cell_renderer_toggle_new ();
  gtk_tree_view_column_pack_start (col, cell, FALSE);
  g_signal_connect (cell, "toggled", G_CALLBACK (toggle_toggled_cb), data);
  gtk_tree_view_column_set_cell_data_func (col, cell, (GtkTreeCellDataFunc) groups_toggle_cell_data_func, data, NULL);


  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, cell, TRUE);
  gtk_tree_view_column_set_cell_data_func (col, cell, groups_name_cell_data_func, NULL, NULL);
  g_object_set (G_OBJECT (cell), "editable", TRUE, NULL);
  g_signal_connect (cell, "edited", G_CALLBACK (groups_name_edited_cb), data);

  cell = koto_cell_renderer_pixbuf_new ();
  g_object_set (G_OBJECT (cell), "stock-id", GTK_STOCK_DELETE, NULL);
  g_signal_connect (G_OBJECT (cell), "activated", G_CALLBACK (delete_renderer_activated_cb), data);
  gtk_tree_view_column_pack_start (col, cell, FALSE);



  w = gtk_button_new_with_label ("Add Group");
  gtk_widget_set_name (w, "moko-contacts-add-detail-button");
  g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (add_groups_clicked_cb), data);
  gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

}

void
free_contacts_groups_page (ContactsData *data)
{
}

void
contacts_groups_page_update (ContactsData *data)
{
  const gchar *s;
  EContact *contact;

  contact = data->contact;

  /* set the title of the page */
  if (contact)
  {
    gchar *markup;
    s = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
    if (s)
    {
      markup = g_markup_printf_escaped ("<b>%s</b>", s);
      gtk_label_set_markup (GTK_LABEL (data->groups_label), markup);
      g_free (markup);
    }
    else
    {
      gtk_label_set_markup (GTK_LABEL (data->groups_label), "<b>Groups</b>");
    }

    /* ref the contact so it doesn't go away when committed */
    g_object_ref (contact);

    gtk_widget_set_sensitive (data->groups_box, TRUE);

  }
  else
  {
    gtk_widget_set_sensitive (data->groups_box, FALSE);
    gtk_label_set_markup (GTK_LABEL (data->groups_label), "<b>Groups</b>");
  }


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

  contact = data->contact;
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

  data->dirty = TRUE;
}

static void
delete_groups_helper (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, DeleteData *data)
{
  EContact *contact;
  EVCardAttribute *attr;

  gtk_tree_model_get (model, iter, COLUMN_CONTACT, &contact, -1);

  attr = e_vcard_get_attribute (E_VCARD (contact), EVC_CATEGORIES);
  if (attr && hito_vcard_attribute_has_value (attr, data->name))
  {
    e_vcard_attribute_remove_value (attr, data->name);
    e_book_async_commit_contact (data->contacts_data->book, contact, NULL, NULL);
  }

}

static void
delete_renderer_activated_cb (KotoCellRendererPixbuf *cell, const char *path, ContactsData *data)
{
  GtkWidget *d;
  HitoGroup *group;
  const gchar *name = NULL;
  GtkTreeIter iter;

  gtk_tree_model_get_iter_from_string (data->groups_liststore, &iter, path);
  gtk_tree_model_get (data->groups_liststore, &iter, 0, &group, -1);

  name = hito_group_get_name (group);

  d = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, "Are you sure you want to delete the group \"%s\" from all contacts?", name);
  gtk_dialog_add_buttons (GTK_DIALOG (d), GTK_STOCK_DELETE, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

  if (gtk_dialog_run (GTK_DIALOG (d)) == GTK_RESPONSE_OK)
  {
    DeleteData delete_data;

    delete_data.name = name;
    delete_data.contacts_data = data;

    gtk_tree_model_foreach (GTK_TREE_MODEL (data->contacts_store),
        (GtkTreeModelForeachFunc) delete_groups_helper, &delete_data);
    hito_group_store_remove_group (HITO_GROUP_STORE (data->groups_liststore), group);
  }

  gtk_widget_destroy (d);

}

static void
add_groups_clicked_cb (GtkWidget *button, ContactsData *data)
{
  const gchar *new_name;
  GtkWidget *d; /* temporary dialog until group rename is implemented */
  GtkWidget *w;

  d = gtk_dialog_new_with_buttons ("New Group", GTK_WINDOW (data->window),
      GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

  w = gtk_entry_new ();
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (d)->vbox), w);
  gtk_widget_show (w);

  if (gtk_dialog_run (GTK_DIALOG (d)) == GTK_RESPONSE_OK)
  {
    EVCardAttribute *attr;

    new_name = gtk_entry_get_text (GTK_ENTRY (w));

    if (new_name && !g_str_equal (new_name, ""))
    {
      attr = e_vcard_get_attribute (E_VCARD (data->contact), EVC_CATEGORIES);
      if (!attr)
      {
        attr = e_vcard_attribute_new (NULL, EVC_CATEGORIES);
        e_vcard_add_attribute (E_VCARD (data->contact), attr);
      }
      e_vcard_attribute_add_value (attr, new_name);
    }

    e_book_async_commit_contact (data->book, data->contact, NULL, NULL);
  }
  gtk_widget_destroy (d);
}
