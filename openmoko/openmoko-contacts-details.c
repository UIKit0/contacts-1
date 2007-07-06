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

#include <config.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>
#include <string.h>

#include "openmoko-contacts.h"
#include "hito-contact-preview.h"
#include "hito-vcard-util.h"
#include "koto-cell-renderer-pixbuf.h"


#define PADDING 6

enum {
  ATTR_POINTER_COLUMN,
  ATTR_NAME_COLUMN,
  ATTR_TYPE_COLUMN,
  ATTR_VALUE_COLUMN
};

static gboolean filter_visible_func (GtkTreeModel *model, GtkTreeIter *iter, gchar *name);
static void edit_toggle_toggled_cb (GtkWidget *button, ContactsData *data);
static void delete_contact_clicked_cb (GtkWidget *button, ContactsData *data);
static void value_renderer_edited_cb (GtkCellRenderer *renderer, gchar *path, gchar *text, gpointer data);
static void type_renderer_edited_cb (GtkCellRenderer *renderer, gchar *path, gchar *text, gpointer data);
static void delete_renderer_activated_cb (KotoCellRendererPixbuf *cell, const char *path, gpointer data);
static void attribute_store_row_changed_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);

static void add_new_telephone (GtkWidget *button, ContactsData *data);
static void add_new_email (GtkWidget *button, ContactsData *data);

static gboolean
filter_visible_func (GtkTreeModel *model, GtkTreeIter *iter, gchar *name)
{
  gchar *value;
  gtk_tree_model_get (model, iter, ATTR_NAME_COLUMN, &value, -1);
  return (value && name && !strcmp (value, name));
}

static void
append_delete_column (GtkTreeView *treeview)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *treeview_column;

  renderer = koto_cell_renderer_pixbuf_new ();
  g_signal_connect (G_OBJECT (renderer), "activated", G_CALLBACK (delete_renderer_activated_cb), gtk_tree_view_get_model (treeview));
  g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_DELETE, NULL);
  treeview_column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
  g_object_set (G_OBJECT (treeview_column), "visible", FALSE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), treeview_column);
}

static void
append_type_column (GtkTreeView *treeview)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *treeview_column;
  GtkListStore *liststore;
  GtkTreeIter iter;

  liststore = gtk_list_store_new (1, G_TYPE_STRING);
  gtk_list_store_insert_with_values (liststore, &iter, 0, 0, "WORK", -1);
  gtk_list_store_insert_with_values (liststore, &iter, 0, 0, "HOME", -1);
  gtk_list_store_insert_with_values (liststore, &iter, 0, 0, "OTHER", -1);
  gtk_list_store_insert_with_values (liststore, &iter, 0, 0, "PREF", -1);

  renderer = gtk_cell_renderer_combo_new();
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (type_renderer_edited_cb), gtk_tree_view_get_model (treeview));
  g_object_set (G_OBJECT (renderer), "model", liststore, "text-column", 0, "has-entry", FALSE, NULL);
  treeview_column = gtk_tree_view_column_new_with_attributes ("", renderer, "text", ATTR_TYPE_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), treeview_column);
}


void
create_contacts_details_page (ContactsData *data)
{

  GtkWidget *box, *hbox, *toolbar, *w, *align;
  GtkToolItem *toolitem;
  GtkListStore *liststore;
  GtkTreeModel *tel_filter, *email_filter;
  GtkCellRenderer *renderer;

  box = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (data->notebook), box, gtk_image_new_from_stock (GTK_STOCK_INFO, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_container_child_set (GTK_CONTAINER (data->notebook), box, "tab-expand", TRUE, "tab-fill", TRUE, NULL);

  toolbar = gtk_toolbar_new ();
  gtk_box_pack_start (GTK_BOX (box), toolbar, FALSE, FALSE, 0);

  toolitem = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_EDIT);
  g_signal_connect (G_OBJECT (toolitem), "toggled", G_CALLBACK (edit_toggle_toggled_cb), data);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 0);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), 1);

  toolitem = gtk_tool_button_new_from_stock (GTK_STOCK_DELETE);
  g_signal_connect (G_OBJECT (toolitem), "clicked", G_CALLBACK (delete_contact_clicked_cb), data);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 2);


  hbox = gtk_hbox_new (FALSE, PADDING);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, PADDING);

  data->photo = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (hbox), data->photo, FALSE, FALSE, 0);

  w = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, PADDING);

  data->fullname = gtk_label_new (NULL);
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), data->fullname);
  gtk_box_pack_start (GTK_BOX (w), align, TRUE, TRUE, 0);

  data->org = gtk_label_new (NULL);
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), data->org);
  gtk_box_pack_start (GTK_BOX (w), align, TRUE, TRUE, 0);


  /* liststore for attributes */
  liststore = gtk_list_store_new (4, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  g_signal_connect (G_OBJECT (liststore), "row-changed", G_CALLBACK (attribute_store_row_changed_cb), NULL);
  data->attribute_liststore = liststore;

  /* telephone entries */
  tel_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (liststore), NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (tel_filter), (GtkTreeModelFilterVisibleFunc) filter_visible_func, EVC_TEL, NULL);

  data->telephone = gtk_tree_view_new_with_model(GTK_TREE_MODEL (tel_filter));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->telephone), FALSE);

  /* delete option column */
  append_delete_column (GTK_TREE_VIEW (data->telephone));

  /* type option column */
  append_type_column (GTK_TREE_VIEW (data->telephone));

  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (value_renderer_edited_cb), tel_filter);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->telephone),
      gtk_tree_view_column_new_with_attributes ("Value", renderer, "text", ATTR_VALUE_COLUMN, NULL));


  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (w), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_container_add (GTK_CONTAINER (w), data->telephone);
  gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);

  /* add phone button */
  w = gtk_button_new_from_stock (GTK_STOCK_ADD);
  g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (add_new_telephone), data);
  g_object_set (G_OBJECT (w), "no-show-all", TRUE, NULL);
  gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
  data->add_telephone_button = w;


  /* email entries */
  email_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (liststore), NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (email_filter), (GtkTreeModelFilterVisibleFunc) filter_visible_func, EVC_EMAIL, NULL);

  data->email = gtk_tree_view_new_with_model(GTK_TREE_MODEL (email_filter));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->email), FALSE);

  /* delete option column */
  append_delete_column (GTK_TREE_VIEW (data->email));

  /* type option column */
  append_type_column (GTK_TREE_VIEW (data->email));

  /* value column */
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (value_renderer_edited_cb), email_filter);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->email),
      gtk_tree_view_column_new_with_attributes ("Value", renderer, "text", ATTR_VALUE_COLUMN, NULL));

  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (w), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_container_add (GTK_CONTAINER (w), data->email);
  gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);

  /* add e-mail button */
  w = gtk_button_new_from_stock (GTK_STOCK_ADD);
  g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (add_new_email), data);
  g_object_set (G_OBJECT (w), "no-show-all", TRUE, NULL);
  gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
  data->add_email_button = w;

}

void
free_contacts_details_page (ContactsData *data)
{
  EContact *old_contact;
  GtkListStore *liststore;
  gboolean dirty;

  /* Clear up any loose ends */

  liststore = data->attribute_liststore;
  dirty = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (liststore), "dirty"));

  if (dirty)
  {
    old_contact = g_object_get_data (G_OBJECT (liststore), "econtact");
    /* TODO: error checking on failure */
    e_book_commit_contact (data->book, old_contact, NULL);
  }

}

static void
contact_photo_size (GdkPixbufLoader * loader, gint width, gint height,
        gpointer user_data)
{
  /* Max height of GTK_ICON_SIZE_DIALOG */
  gint iconwidth, iconheight;
  gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &iconwidth, &iconheight);
  gdk_pixbuf_loader_set_size (loader, width / ((gdouble) height / iconheight), iconheight);
}

void
contacts_details_page_set_contact (ContactsData *data, EContact *contact)
{
  GList *attributes, *a;
  GtkListStore *liststore;
  gboolean photo_set = FALSE;
  gboolean dirty;
  EContact *old_contact;

  liststore = data->attribute_liststore;
  dirty = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (liststore), "dirty"));

  if (dirty)
  {
    old_contact = g_object_get_data (G_OBJECT (liststore), "econtact");
    /* TODO: error checking on failure */
    e_book_commit_contact (data->book, old_contact, NULL);
  }

  /* set up references to current contact and ebook */
  g_object_set_data (G_OBJECT (liststore), "econtact", contact);
  g_object_set_data (G_OBJECT (liststore), "dirty", GINT_TO_POINTER (FALSE));

  gtk_list_store_clear (liststore);
  gtk_label_set_text (GTK_LABEL (data->fullname), NULL);
  gtk_label_set_text (GTK_LABEL (data->org), NULL);

  attributes = e_vcard_get_attributes (E_VCARD (contact));
  for (a = attributes; a; a = g_list_next (a))
  {
    GtkTreeIter iter;
    const gchar *name, *value;
    name = e_vcard_attribute_get_name (a->data);
    value = hito_vcard_attribute_get_value_string (a->data);

    if (!strcmp (name, EVC_FN))
    {
      gchar *markup;
      markup = g_markup_printf_escaped ("<big><b>%s</b></big>", value);
      gtk_label_set_markup (GTK_LABEL (data->fullname), markup);
      g_free (markup);
      continue;
    }
    if (!strcmp (name, EVC_ORG))
    {
      gtk_label_set_text (GTK_LABEL (data->org), value);
      continue;
    }
    if (!strcmp (name, EVC_PHOTO))
    {
      GdkPixbufLoader *ploader;
      guchar *buf;
      gsize size;
      buf = g_base64_decode (value, &size);
      ploader = gdk_pixbuf_loader_new ();
      g_signal_connect (G_OBJECT (ploader), "size-prepared", G_CALLBACK (contact_photo_size),  NULL);

      gdk_pixbuf_loader_write (ploader, buf, size, NULL);
      gtk_image_set_from_pixbuf (GTK_IMAGE (data->photo), g_object_ref (gdk_pixbuf_loader_get_pixbuf (ploader)));
      gdk_pixbuf_loader_close (ploader, NULL);
      g_object_unref (ploader);
      photo_set = TRUE;
    }

    gtk_list_store_insert_with_values (liststore, &iter, 0,
        ATTR_POINTER_COLUMN, a->data,
        ATTR_NAME_COLUMN, name,
        ATTR_TYPE_COLUMN, hito_vcard_attribute_get_type (a->data),
        ATTR_VALUE_COLUMN, value,
        -1);
  }

  if (!photo_set)
    gtk_image_set_from_stock (GTK_IMAGE (data->photo), GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_DIALOG);
}



static void
edit_toggle_toggled_cb (GtkWidget *button, ContactsData *data)
{
  GtkTreeViewColumn *col;
  GList *list;
  gboolean editing;

  editing = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (button));

  /* FIXME: these should be #defined (or similar)
   * column 0 = delete
   * column 1 = type
   * column 2 = value
   */

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->telephone), 0);
  g_object_set (G_OBJECT (col), "visible", editing, NULL);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->email), 0);
  g_object_set (G_OBJECT (col), "visible", editing, NULL);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->telephone), 1);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editing, NULL);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->telephone), 2);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editing, NULL);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->email), 1);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editing, NULL);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->email), 2);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editing, NULL);

  g_object_set (G_OBJECT (data->add_telephone_button), "visible", editing, NULL);
  g_object_set (G_OBJECT (data->add_email_button), "visible", editing, NULL);

  /* remove current focus to close any active edits */
  if (!editing)
    gtk_window_set_focus (GTK_WINDOW (data->window), NULL);
}


static void
delete_contact_clicked_cb (GtkWidget *button, ContactsData *data)
{
  EContact *card;
  GtkWidget *dialog;
  const gchar *name = NULL;

  card = g_object_get_data (G_OBJECT (data->attribute_liststore), "econtact");
  name = e_contact_get_const (card, E_CONTACT_FULL_NAME);

  dialog = gtk_message_dialog_new (GTK_WINDOW (data->window),
      GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "Are you sure you want to delete \"%s\" from your address book", name);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_DELETE, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    GError *err = NULL;
    GtkWidget *err_message;
    e_book_remove_contact (data->book, e_contact_get_const (card, E_CONTACT_UID), &err);
    gtk_widget_destroy (dialog);
    if (err)
    {
      err_message = gtk_message_dialog_new (GTK_WINDOW (data->window), 
          GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
          "The following error occured while attempting to delete the contact:\n\"%s\"", err->message);
      gtk_dialog_run (GTK_DIALOG (err_message));
      gtk_widget_destroy (err_message);
    }
  }
  else
  {
    gtk_widget_destroy (dialog);
  }

  /* FIXME: return to contact list or something ... */
}

static void
value_renderer_edited_cb (GtkCellRenderer *renderer, gchar *path, gchar *text, gpointer data)
{
  GtkTreeIter filter_iter, model_iter;
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (data);
  GtkTreeModel *model;

  gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (filter), &filter_iter, path);
  gtk_tree_model_filter_convert_iter_to_child_iter (filter, &model_iter, &filter_iter);
  model = gtk_tree_model_filter_get_model (filter);

  gtk_list_store_set (GTK_LIST_STORE (model), &model_iter, ATTR_VALUE_COLUMN, text, -1);
}


static void
type_renderer_edited_cb (GtkCellRenderer *renderer, gchar *path, gchar *text, gpointer data)
{
  GtkTreeIter filter_iter, model_iter;
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (data);
  GtkTreeModel *model;

  gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (filter), &filter_iter, path);
  gtk_tree_model_filter_convert_iter_to_child_iter (filter, &model_iter, &filter_iter);
  model = gtk_tree_model_filter_get_model (filter);

  gtk_list_store_set (GTK_LIST_STORE (model), &model_iter, ATTR_TYPE_COLUMN, text, -1);
}


static void
delete_renderer_activated_cb (KotoCellRendererPixbuf *cell, const char *path, gpointer data)
{
  EVCardAttribute *attr;
  EVCard *card;
  GtkTreeIter iter, child_iter;
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (data);
  GtkTreeModel *model;

  model = gtk_tree_model_filter_get_model (filter);

  /* remove attribute from contact */
  gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (filter), &iter, path);
  gtk_tree_model_get (GTK_TREE_MODEL (filter), &iter, ATTR_POINTER_COLUMN, &attr, -1);
  card = E_VCARD (g_object_get_data (G_OBJECT (model), "econtact"));

  e_vcard_remove_attribute (card, attr);

  /* remove attribute row from model */
  gtk_tree_model_filter_convert_iter_to_child_iter (filter, &child_iter, &iter);
  gtk_list_store_remove (GTK_LIST_STORE (model), &child_iter);

  /* mark liststore as "dirty" so we can commit the contact later */
  g_object_set_data (G_OBJECT (model), "dirty", GINT_TO_POINTER (TRUE));
}

static void
attribute_store_row_changed_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  EVCardAttribute *attr;
  gchar *value;

  gtk_tree_model_get (model, iter, ATTR_POINTER_COLUMN, &attr, ATTR_VALUE_COLUMN, &value, -1);

  /* remove old values and add new one */
  e_vcard_attribute_remove_values (attr);
  /* TODO: check for multi valued string */
  e_vcard_attribute_add_value (attr, value);


  gtk_tree_model_get (model, iter, ATTR_TYPE_COLUMN, &value, -1);
  hito_vcard_attribute_set_type (attr, value);

  /* mark liststore as "dirty" so we can commit the contact later */
  g_object_set_data (G_OBJECT (model), "dirty", GINT_TO_POINTER (TRUE));

}

static void
add_new_attribute (GtkTreeView *treeview, ContactsData *data, const gchar *attr_name, const gchar *attr_type)
{
  EVCardAttribute *attr;
  GtkTreeIter child_iter, filter_iter;
  EVCard *contact;
  GtkTreeViewColumn *col;
  GtkTreePath *filter_path;
  GtkTreeModel *filter;

  attr = e_vcard_attribute_new (NULL, EVC_EMAIL);

  contact = g_object_get_data (G_OBJECT (data->attribute_liststore), "econtact");
  e_vcard_add_attribute (contact, attr);

  gtk_list_store_insert_with_values (data->attribute_liststore, &child_iter, 0, ATTR_POINTER_COLUMN, attr, ATTR_NAME_COLUMN, attr_name, ATTR_TYPE_COLUMN, attr_type, -1);

  filter = gtk_tree_view_get_model (treeview);
  gtk_tree_model_filter_convert_child_iter_to_iter (GTK_TREE_MODEL_FILTER (filter),  &filter_iter, &child_iter);

  col = gtk_tree_view_get_column (treeview, 2);

  filter_path = gtk_tree_model_get_path (filter, &filter_iter);
  gtk_tree_view_set_cursor (treeview, filter_path, col, TRUE);
  gtk_widget_activate (GTK_WIDGET (treeview));

  gtk_tree_path_free (filter_path);
}

static void
add_new_telephone (GtkWidget *button, ContactsData *data)
{
  add_new_attribute (GTK_TREE_VIEW (data->telephone), data, EVC_TEL, "WORK");
}

static void
add_new_email (GtkWidget *button, ContactsData *data)
{
  add_new_attribute (GTK_TREE_VIEW (data->email), data, EVC_EMAIL, "WORK");
}


