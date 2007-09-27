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
#include <moko-finger-scroll.h>
#include <moko-stock.h>
#include <moko-hint-entry.h>

#include "openmoko-contacts.h"
#include "hito-contact-preview.h"
#include "hito-vcard-util.h"
#include "koto-cell-renderer-pixbuf.h"

#if GLIB_MINOR_VERSION < 12
#include "gbase64.h"
#endif


enum {
  ATTR_POINTER_COLUMN,
  ATTR_NAME_COLUMN,
  ATTR_TYPE_COLUMN,
  ATTR_VALUE_COLUMN
};


enum {
  TREE_DEL_COLUMN = 0,
  TREE_TYPE_COLUMN,
  TREE_ICON_COLUMN,
  TREE_VALUE_COLUMN
};

struct _AttributeName {
  gchar *vcard_name;
  gchar *pretty_name;
};

struct _AttributeName attr_names[] = {
  {EVC_TEL, "Telephone"},
  {EVC_EMAIL, "E-Mail"},
  {EVC_FN, "Fullname"},
  {EVC_ORG, "Organization"}
};

enum {
  ATTR_TEL = 0,
  ATTR_EMAIL,
  ATTR_FN,
  ATTR_ORG
};

static gboolean filter_visible_func (GtkTreeModel *model, GtkTreeIter *iter, gchar *name);
static void edit_toggle_toggled_cb (GtkWidget *button, ContactsData *data);
static void delete_contact_clicked_cb (GtkWidget *button, ContactsData *data);
static void value_renderer_edited_cb (GtkCellRenderer *renderer, gchar *path, gchar *text, gpointer data);
static void type_renderer_edited_cb (GtkCellRenderer *renderer, gchar *path, gchar *text, gpointer data);
static void delete_renderer_activated_cb (KotoCellRendererPixbuf *cell, const char *path, ContactsData *data);
static void attribute_store_row_changed_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, ContactsData *data);
static void fullname_changed_cb (GtkWidget *entry, ContactsData *data);
static void org_changed_cb (GtkWidget *entry, ContactsData *data);

static void add_new_telephone (GtkWidget *button, ContactsData *data);
static void add_new_email (GtkWidget *button, ContactsData *data);

static void free_liststore_data (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);

static gboolean
filter_visible_func (GtkTreeModel *model, GtkTreeIter *iter, gchar *name)
{
  gchar *value;
  gboolean result;
  gtk_tree_model_get (model, iter, ATTR_NAME_COLUMN, &value, -1);
  result = (value && name && !strcmp (value, name));
  g_free (value);
  return result;
}

static void
append_delete_column (GtkTreeView *treeview, ContactsData *data)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *treeview_column;

  renderer = koto_cell_renderer_pixbuf_new ();
  g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_DELETE, NULL);
  /* we need to know what treeview this renderer is associated with for the
   * "activated" callback */
  g_object_set_data (G_OBJECT (renderer), "treeview", treeview);
  g_signal_connect (G_OBJECT (renderer), "activated", G_CALLBACK (delete_renderer_activated_cb), data);

  treeview_column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
  g_object_set (G_OBJECT (treeview_column), "visible", FALSE, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), treeview_column, TREE_DEL_COLUMN);
}

static void
append_type_column (GtkTreeView *treeview)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *treeview_column;
  GtkListStore *liststore;
  GtkTreeIter iter;

  /* FIXME: this should be translatable somehow */
  liststore = gtk_list_store_new (1, G_TYPE_STRING);
  gtk_list_store_insert_with_values (liststore, &iter, 0, 0, "Work", -1);
  gtk_list_store_insert_with_values (liststore, &iter, 0, 0, "Home", -1);
  gtk_list_store_insert_with_values (liststore, &iter, 0, 0, "Other", -1);
  gtk_list_store_insert_with_values (liststore, &iter, 0, 0, "Pref", -1);

  renderer = gtk_cell_renderer_combo_new();
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (type_renderer_edited_cb), gtk_tree_view_get_model (treeview));
  g_object_set (G_OBJECT (renderer), "model", liststore, "text-column", 0, "has-entry", FALSE, NULL);
  treeview_column = gtk_tree_view_column_new_with_attributes ("", renderer, "text", ATTR_TYPE_COLUMN, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), treeview_column, TREE_TYPE_COLUMN);
}


static void
append_icon_column (GtkTreeView *treeview, gchar *stock_id)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *treeview_column;

  renderer = gtk_cell_renderer_pixbuf_new();
  g_object_set (G_OBJECT (renderer), "stock-id", stock_id, NULL);
  treeview_column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (treeview_column, renderer, FALSE);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), treeview_column, TREE_ICON_COLUMN);
}


void
create_contacts_details_page (ContactsData *data)
{

  GtkWidget *box, *hbox, *toolbar, *w, *sw, *vb, *viewport, *main_vbox;
  GtkToolItem *toolitem;
  GtkListStore *liststore;
  GtkTreeModel *tel_filter, *email_filter;
  GtkCellRenderer *renderer;

  box = gtk_vbox_new (FALSE, 0);

  contacts_notebook_add_page_with_icon (data->notebook, box, GTK_STOCK_FILE);

  toolbar = gtk_toolbar_new ();
  gtk_box_pack_start (GTK_BOX (box), toolbar, FALSE, FALSE, 0);

  toolitem = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_EDIT);
  g_signal_connect (G_OBJECT (toolitem), "toggled", G_CALLBACK (edit_toggle_toggled_cb), data);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 0);
  data->edit_toggle = toolitem;

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), 1);

  toolitem = gtk_tool_button_new_from_stock (GTK_STOCK_DELETE);
  g_signal_connect (G_OBJECT (toolitem), "clicked", G_CALLBACK (delete_contact_clicked_cb), data);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 2);


  main_vbox = gtk_vbox_new (FALSE, PADDING);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), PADDING);

  sw = moko_finger_scroll_new ();
  gtk_box_pack_start (GTK_BOX (box), sw, TRUE, TRUE, PADDING);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (sw), viewport);
  gtk_container_add (GTK_CONTAINER (viewport), main_vbox);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);


  hbox = gtk_hbox_new (FALSE, PADDING);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  data->photo = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (hbox), data->photo, FALSE, FALSE, 0);

  w = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, PADDING);

  data->fullname = moko_hint_entry_new ("Name");
  gtk_box_pack_start (GTK_BOX (w), data->fullname, TRUE, TRUE, 0);
  g_signal_connect (data->fullname, "changed", G_CALLBACK (fullname_changed_cb), data);

  data->org = moko_hint_entry_new ("Organization");
  gtk_box_pack_start (GTK_BOX (w), data->org, TRUE, TRUE, 0);
  g_signal_connect (data->org, "changed", G_CALLBACK (org_changed_cb), data);


  /* liststore for attributes */
  liststore = gtk_list_store_new (4, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  g_signal_connect (G_OBJECT (liststore), "row-changed", G_CALLBACK (attribute_store_row_changed_cb), data);
  data->attribute_liststore = liststore;

  /* telephone entries */
  vb = gtk_vbox_new (0, FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox), vb, FALSE, FALSE, 0);

  tel_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (liststore), NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (tel_filter),
      (GtkTreeModelFilterVisibleFunc) filter_visible_func, EVC_TEL, NULL);

  w = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (w), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vb), w, FALSE, FALSE, 0);

  data->telephone = gtk_tree_view_new_with_model(GTK_TREE_MODEL (tel_filter));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->telephone), FALSE);
  gtk_container_add (GTK_CONTAINER (w), data->telephone);

  /* delete option column */
  append_delete_column (GTK_TREE_VIEW (data->telephone), data);

  /* type option column */
  append_type_column (GTK_TREE_VIEW (data->telephone));

  /* icon column */
  append_icon_column (GTK_TREE_VIEW (data->telephone), MOKO_STOCK_CONTACT_PHONE);

  /* value column */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (value_renderer_edited_cb), tel_filter);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (data->telephone),
      gtk_tree_view_column_new_with_attributes ("Value", renderer, "text", ATTR_VALUE_COLUMN, NULL), TREE_VALUE_COLUMN);


  /* add phone button */
  w = gtk_button_new_with_label ("Add Phone Number");
  gtk_widget_set_name (w, "moko-contacts-add-detail-button");
  g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (add_new_telephone), data);
  g_object_set (G_OBJECT (w), "no-show-all", TRUE, NULL);
  gtk_box_pack_start (GTK_BOX (vb), w, FALSE, FALSE, 0);
  data->add_telephone_button = w;


  /* email entries */
  vb = gtk_vbox_new (0, FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox), vb, FALSE, FALSE, 0);

  email_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (liststore), NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (email_filter), (GtkTreeModelFilterVisibleFunc) filter_visible_func, EVC_EMAIL, NULL);

  w = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (w), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vb), w, FALSE, FALSE, 0);

  data->email = gtk_tree_view_new_with_model(GTK_TREE_MODEL (email_filter));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->email), FALSE);
  gtk_container_add (GTK_CONTAINER (w), data->email);

  /* delete option column */
  append_delete_column (GTK_TREE_VIEW (data->email), data);

  /* type option column */
  append_type_column (GTK_TREE_VIEW (data->email));

  /* icon column */
  append_icon_column (GTK_TREE_VIEW (data->email), MOKO_STOCK_CONTACT_EMAIL);


  /* value column */
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (value_renderer_edited_cb), email_filter);
  g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (data->email),
      gtk_tree_view_column_new_with_attributes ("Value", renderer, "text", ATTR_VALUE_COLUMN, NULL), TREE_VALUE_COLUMN);


  /* add e-mail button */
  w = gtk_button_new_with_label ("Add E-Mail Address");
  gtk_widget_set_name (w, "moko-contacts-add-detail-button");
  g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (add_new_email), data);
  g_object_set (G_OBJECT (w), "no-show-all", TRUE, NULL);
  gtk_box_pack_start (GTK_BOX (vb), w, FALSE, FALSE, 0);
  data->add_email_button = w;

  /* make sure everything is in the correct state */
  edit_toggle_toggled_cb (GTK_WIDGET (data->edit_toggle), data);
}

void
free_contacts_details_page (ContactsData *data)
{
  /* free data referenced in the attribute liststore */
  gtk_tree_model_foreach (GTK_TREE_MODEL (data->attribute_liststore), (GtkTreeModelForeachFunc) free_liststore_data, NULL);

  /* unref the attribute list */
  g_object_unref (data->attribute_liststore);
}

void
contacts_details_page_set_editable (ContactsData *data, gboolean editing)
{
  gboolean tb_state;

  tb_state = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (data->edit_toggle));

  /* change active state only if current state is different to new state */
  if (tb_state ^ editing)
  {
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (data->edit_toggle), editing);
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

static void
free_liststore_data (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  gchar *value = NULL, *type = NULL;
  gtk_tree_model_get (model, iter, ATTR_VALUE_COLUMN, &value, ATTR_TYPE_COLUMN, &type, -1);
  g_free (value);
  g_free (type);
}

void
contacts_details_page_update (ContactsData *data)
{
  GList *attributes, *a;
  GtkListStore *liststore;
  gboolean photo_set = FALSE, fn_set = FALSE, org_set = FALSE;

  liststore = data->attribute_liststore;

  data->detail_page_loading = TRUE;

  /* ensure the default mode is view, not edit */
  contacts_details_page_set_editable (data, FALSE);


  gtk_tree_model_foreach (GTK_TREE_MODEL (liststore), (GtkTreeModelForeachFunc) free_liststore_data, NULL);
  gtk_list_store_clear (liststore);


  if (data->contact)
    attributes = e_vcard_get_attributes (E_VCARD (data->contact));
  else
    attributes = NULL;
  for (a = attributes; a; a = g_list_next (a))
  {
    GtkTreeIter iter;
    const gchar *name, *value;
    name = e_vcard_attribute_get_name (a->data);
    value = hito_vcard_attribute_get_value_string (a->data);

    if (!strcmp (name, EVC_FN))
    {
      moko_hint_entry_set_text (MOKO_HINT_ENTRY (data->fullname), value);
      fn_set = TRUE;
      continue;
    }
    if (!strcmp (name, EVC_ORG))
    {
      moko_hint_entry_set_text (MOKO_HINT_ENTRY (data->org), value);
      org_set = TRUE;
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
      gdk_pixbuf_loader_close (ploader, NULL);
      gtk_image_set_from_pixbuf (GTK_IMAGE (data->photo), g_object_ref (gdk_pixbuf_loader_get_pixbuf (ploader)));
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
  if (!org_set)
    moko_hint_entry_set_text (MOKO_HINT_ENTRY (data->org), "");
  if (!fn_set)
    moko_hint_entry_set_text (MOKO_HINT_ENTRY (data->fullname), "");

  data->detail_page_loading = FALSE;
}



static void
edit_toggle_toggled_cb (GtkWidget *button, ContactsData *data)
{
  GtkTreeViewColumn *col;
  GtkTreeSelection *sel;
  GList *list;
  gboolean editing;

  editing = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (data->edit_toggle));

  g_object_set (G_OBJECT (data->telephone), "can-focus", editing, NULL);
  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->telephone), TREE_DEL_COLUMN);
  g_object_set (G_OBJECT (col), "visible", editing, NULL);

  g_object_set (G_OBJECT (data->email), "can-focus", editing, NULL);
  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->email), TREE_DEL_COLUMN);
  g_object_set (G_OBJECT (col), "visible", editing, NULL);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->telephone), TREE_TYPE_COLUMN);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editing, NULL);
  g_list_free (list);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->telephone), TREE_VALUE_COLUMN);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editing, NULL);
  g_list_free (list);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->email), TREE_TYPE_COLUMN);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editing, NULL);
  g_list_free (list);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (data->email), TREE_VALUE_COLUMN);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editing, NULL);
  g_list_free (list);

  g_object_set (G_OBJECT (data->add_telephone_button), "visible", editing, NULL);
  g_object_set (G_OBJECT (data->add_email_button), "visible", editing, NULL);

  gtk_entry_set_has_frame (GTK_ENTRY (data->fullname), editing);
  g_object_set (G_OBJECT (data->fullname), "editable", editing, NULL);

  gtk_entry_set_has_frame (GTK_ENTRY (data->org), editing);
  g_object_set (G_OBJECT (data->org), "editable", editing, NULL);

  if (editing)
  {
    if (!moko_hint_entry_is_empty (MOKO_HINT_ENTRY (data->fullname)))
    {
      gtk_widget_modify_text (data->fullname, GTK_STATE_NORMAL, NULL);
      gtk_widget_modify_base (data->fullname, GTK_STATE_NORMAL, NULL);
    }

    if (!moko_hint_entry_is_empty (MOKO_HINT_ENTRY (data->org)))
    {
      gtk_widget_modify_text (data->org, GTK_STATE_NORMAL, NULL);
      gtk_widget_modify_base (data->org, GTK_STATE_NORMAL, NULL);
    }


    /* make sure fullname and org are visible */
    gtk_widget_show (data->fullname);
    gtk_widget_show (data->org);

    /* ensure selection is possible in edit mode - cell editing is not possible
     * without it
     */
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->telephone));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->email));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);

  }
  else
  {
    if (moko_hint_entry_is_empty (MOKO_HINT_ENTRY (data->fullname)))
      gtk_widget_hide (data->fullname);
    else
    {
      gtk_widget_modify_text (data->fullname, GTK_STATE_NORMAL, &data->org->style->fg[GTK_STATE_NORMAL]);
      gtk_widget_modify_base (data->fullname, GTK_STATE_NORMAL, &data->org->style->bg[GTK_STATE_NORMAL]);
    }

    if (moko_hint_entry_is_empty (MOKO_HINT_ENTRY (data->org)))
      gtk_widget_hide (data->org);
    else
    {
      gtk_widget_modify_text (data->org, GTK_STATE_NORMAL, &data->org->style->fg[GTK_STATE_NORMAL]);
      gtk_widget_modify_base (data->org, GTK_STATE_NORMAL, &data->org->style->bg[GTK_STATE_NORMAL]);
    }




    /* remove current focus to close any active edits */
    gtk_window_set_focus (GTK_WINDOW (data->window), NULL);


    /* disable selection when not in editing mode */
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->telephone));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_NONE);

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->email));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_NONE);
  }

}


static void
delete_contact_clicked_cb (GtkWidget *button, ContactsData *data)
{
  EContact *card;
  GtkWidget *dialog;
  const gchar *name = NULL;

  card = data->contact;
  name = e_contact_get_const (card, E_CONTACT_FULL_NAME);

  dialog = gtk_message_dialog_new (GTK_WINDOW (data->window),
      GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "Are you sure you want to remove \"%s\" from your address book?", name);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_DELETE, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    GError *err = NULL;
    GtkWidget *err_message;
    const gchar *uid;
    uid = e_contact_get_const (card, E_CONTACT_UID);
    if (uid)
    {
      e_book_remove_contact (data->book, uid, &err);
    }
    gtk_widget_destroy (dialog);
    if (err)
    {
      err_message = gtk_message_dialog_new (GTK_WINDOW (data->window), 
          GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
          "The following error occured while attempting to delete the contact:\n\"%s\"", err->message);
      gtk_dialog_run (GTK_DIALOG (err_message));
      gtk_widget_destroy (err_message);
    }
    else
    {
      /* return to contact list */
      gtk_notebook_set_current_page (GTK_NOTEBOOK (data->notebook), LIST_PAGE_NUM);
    }
  }
  else
  {
    gtk_widget_destroy (dialog);
  }

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
delete_renderer_activated_cb (KotoCellRendererPixbuf *cell, const char *path, ContactsData *data)
{
  EVCardAttribute *attr;
  EVCard *card;
  GtkTreeIter iter, child_iter;
  GtkTreeModelFilter *filter;
  GtkTreeModel *model;
  GtkTreeView *treeview;

  treeview = g_object_get_data (G_OBJECT (cell), "treeview");

  /* the model on the treeview is a filter */
  filter = GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview)));
  model = gtk_tree_model_filter_get_model (filter);

  /* remove attribute from contact */
  gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (filter), &iter, path);
  gtk_tree_model_get (GTK_TREE_MODEL (filter), &iter, ATTR_POINTER_COLUMN, &attr, -1);
  card = E_VCARD (data->contact);

  e_vcard_remove_attribute (card, attr);

  /* remove attribute row from model */
  gtk_tree_model_filter_convert_iter_to_child_iter (filter, &child_iter, &iter);
  gtk_list_store_remove (GTK_LIST_STORE (model), &child_iter);

  data->dirty = TRUE;
}

static void
attribute_store_row_changed_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, ContactsData *data)
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

  data->dirty = TRUE;

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

  attr = e_vcard_attribute_new (NULL, attr_name);

  contact = E_VCARD (data->contact);
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
  add_new_attribute (GTK_TREE_VIEW (data->telephone), data, EVC_TEL, "Work");
}

static void
add_new_email (GtkWidget *button, ContactsData *data)
{
  add_new_attribute (GTK_TREE_VIEW (data->email), data, EVC_EMAIL, "Work");
}

static void
attribute_changed (const gchar *attr_name, const gchar *new_val, ContactsData *data)
{
  EVCard *card;
  EVCardAttribute *attr;

  /* don't set the attribute if we are still loading it */
  if (data->detail_page_loading)
    return;

  card = E_VCARD (data->contact);
  attr = e_vcard_get_attribute (card, attr_name);

  if (!attr)
  {
    attr = e_vcard_attribute_new (NULL, attr_name);
    e_vcard_add_attribute (card, attr);
  }

  e_vcard_attribute_remove_values (attr);

  /* FIXME: this is not dealing with multi values yet */
  e_vcard_attribute_add_value (attr, new_val);

  data->dirty = TRUE;
}

static void
fullname_changed_cb (GtkWidget *entry, ContactsData *data)
{
  const gchar *new_val;

  new_val = gtk_entry_get_text (GTK_ENTRY (entry));

  if (moko_hint_entry_is_empty (MOKO_HINT_ENTRY (entry)))
    return;

  attribute_changed (EVC_FN, new_val, data);
}

static void
org_changed_cb (GtkWidget *entry, ContactsData *data)
{
  const gchar *new_val;

  new_val = gtk_entry_get_text (GTK_ENTRY (entry));

  if (moko_hint_entry_is_empty (MOKO_HINT_ENTRY (entry)))
    return;

  attribute_changed (EVC_ORG, new_val, data);
}


