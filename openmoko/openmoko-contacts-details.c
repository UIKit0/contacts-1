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


#define PADDING 6

enum {
  ATTR_POINTER_COLUMN,
  ATTR_NAME_COLUMN,
  ATTR_TYPE_COLUMN,
  ATTR_VALUE_COLUMN
};

static gboolean filter_visible_func (GtkTreeModel *model, GtkTreeIter *iter, gchar *name);
static void edit_toggle_toggled_cb (GtkWidget *button, ContactsData *data);
static void value_renderer_edited_cb (GtkCellRenderer *renderer, gchar *path, gchar *text, gpointer data);

static void attribute_store_row_changed_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);

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

  renderer = gtk_cell_renderer_pixbuf_new();
  g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_DELETE, NULL);
  treeview_column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
  g_object_set (G_OBJECT (treeview_column), "visible", FALSE, NULL);
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
  GtkTreeViewColumn *treeview_column;

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

  /* telephone entries */
  tel_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (liststore), NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (tel_filter), (GtkTreeModelFilterVisibleFunc) filter_visible_func, EVC_TEL, NULL);

  data->telephone = gtk_tree_view_new_with_model(GTK_TREE_MODEL (tel_filter));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->telephone), FALSE);

  /* delete option column */
  append_delete_column (GTK_TREE_VIEW (data->telephone));

  gtk_tree_view_append_column (GTK_TREE_VIEW (data->telephone),
      gtk_tree_view_column_new_with_attributes ("Type", gtk_cell_renderer_text_new(), "text", ATTR_TYPE_COLUMN, NULL));

  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (value_renderer_edited_cb), tel_filter);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->telephone),
      gtk_tree_view_column_new_with_attributes ("Value", renderer, "text", ATTR_VALUE_COLUMN, NULL));


  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (w), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_container_set_border_width (GTK_CONTAINER (w), PADDING);
  gtk_container_add (GTK_CONTAINER (w), data->telephone);
  gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);


  /* email entries */
  email_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (liststore), NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (email_filter), (GtkTreeModelFilterVisibleFunc) filter_visible_func, EVC_EMAIL, NULL);

  data->email = gtk_tree_view_new_with_model(GTK_TREE_MODEL (email_filter));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->email), FALSE);

  /* delete option column */
  append_delete_column (GTK_TREE_VIEW (data->email));

  /* type selector column */
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->email),
      gtk_tree_view_column_new_with_attributes ("Type", gtk_cell_renderer_text_new(), "text", ATTR_TYPE_COLUMN, NULL));

  /* value column */
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (value_renderer_edited_cb), email_filter);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->email),
      gtk_tree_view_column_new_with_attributes ("Value", renderer, "text", ATTR_VALUE_COLUMN, NULL));

  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (w), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_container_set_border_width (GTK_CONTAINER (w), PADDING);
  gtk_container_add (GTK_CONTAINER (w), data->email);
  gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);


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

  liststore = GTK_LIST_STORE (gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (GTK_TREE_VIEW (data->telephone)))));
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

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->telephone), 2);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editing, NULL);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->email), 2);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editing, NULL);

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
attribute_store_row_changed_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  EVCardAttribute *attr;
  gchar *value;

  gtk_tree_model_get (model, iter, ATTR_POINTER_COLUMN, &attr, ATTR_VALUE_COLUMN, &value, -1);

  /* remove old values and add new one */
  e_vcard_attribute_remove_values (attr);
  /* TODO: check for multi valued string */
  e_vcard_attribute_add_value (attr, value);

  /* TODO: commit contact somewhere... */

}
