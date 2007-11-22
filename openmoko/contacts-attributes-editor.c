/*
 * OpenMoko Contacts
 *
 * Copyright (C) 2007 OpenMoko Inc.
 * 
 * hito is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * hito is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <moko-stock.h>

#include "contacts-attributes-editor.h"
#include "koto-cell-renderer-pixbuf.h"

#define DEFAULT_NEW_ATTRIBUTE_TYPE "Work"

G_DEFINE_TYPE (ContactsAttributesEditor, contacts_attributes_editor, GTK_TYPE_VBOX);

enum
{
  PROP_0,
  PROP_ATTR_LIST,
  PROP_FILTER
};

enum {
  TREE_DEL_COLUMN = 0,
  TREE_TYPE_COLUMN,
  TREE_ICON_COLUMN,
  TREE_VALUE_COLUMN
};

typedef struct _ContactsAttributesEditorPriv ContactsAttributesEditorPriv;

struct _ContactsAttributesEditorPriv
{
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *treeview;
  
  GtkTreeModel *filter;
  GtkTreeModel *store;
  
  gchar *stock_id;
  gchar *filter_string;
  
  GtkCellRenderer *icon_renderer;
};

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CONTACTS_TYPE_ATTRIBUTES_EDITOR, ContactsAttributesEditorPriv))


static gboolean
filter_visible_func (GtkTreeModel *model, GtkTreeIter *iter, ContactsAttributesEditorPriv *priv)
{
  gchar *value;
  gboolean result;
  gtk_tree_model_get (model, iter, ATTR_NAME_COLUMN, &value, -1);
  result = (priv->filter_string && value && g_str_equal (value, priv->filter_string));
  g_free (value);
  return result;
}

static void
update_visible_treeviews (ContactsAttributesEditor *self)
{
  ContactsAttributesEditorPriv *priv = GET_PRIVATE (self);
  GtkTreeIter foo;

  /* this hides the treeview completely when there are no rows to display
   * (actually hides the parent since the parent draws the frame)
   */

  if (!gtk_tree_model_get_iter_first (priv->filter, &foo))
    gtk_widget_hide (priv->frame);
  else
    gtk_widget_show (priv->frame);
}

static void
value_renderer_edited_cb (GtkCellRenderer *renderer, gchar *path, gchar *text, ContactsAttributesEditor *self)
{
  ContactsAttributesEditorPriv *priv = GET_PRIVATE (self);
  GtkTreeIter filter_iter, model_iter;

  gtk_tree_model_get_iter_from_string (priv->filter, &filter_iter, path);
  gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (priv->filter), &model_iter, &filter_iter);

  if (g_str_equal (text, ""))
  {
    contacts_attribute_store_remove (CONTACTS_ATTRIBUTE_STORE (priv->store), &model_iter);
  }
  else
  {
    gtk_list_store_set (GTK_LIST_STORE (priv->store), &model_iter, ATTR_VALUE_COLUMN, text, -1);
  }
}


static void
type_renderer_edited_cb (GtkCellRenderer *renderer, gchar *path, gchar *text, ContactsAttributesEditor *self)
{
  ContactsAttributesEditorPriv *priv = GET_PRIVATE (self);
  GtkTreeIter filter_iter, model_iter;

  gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (priv->filter), &filter_iter, path);
  gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (priv->filter),
                                                    &model_iter, &filter_iter);
  gtk_list_store_set (GTK_LIST_STORE (priv->store), &model_iter, ATTR_TYPE_COLUMN, text, -1);
}


static void
delete_renderer_activated_cb (KotoCellRendererPixbuf *cell, const char *path, ContactsAttributesEditor *self)
{
  ContactsAttributesEditorPriv *priv = GET_PRIVATE (self);
  GtkTreeIter iter, child_iter;

  /* remove attribute row from model */
  gtk_tree_model_get_iter_from_string (priv->filter, &iter, path);
  gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (priv->filter),
                                                    &child_iter, &iter);
  contacts_attribute_store_remove (CONTACTS_ATTRIBUTE_STORE (priv->store), &child_iter);
}


static void
add_new_attribute (GtkWidget *widget, ContactsAttributesEditor *self)
{
  ContactsAttributesEditorPriv *priv = GET_PRIVATE (self);
  EVCardAttribute *attr;
  GtkTreeIter child_iter, filter_iter;
  GtkTreeViewColumn *col;
  GtkTreePath *filter_path;

  attr = e_vcard_attribute_new (NULL, priv->filter_string);

  gtk_list_store_insert_with_values (GTK_LIST_STORE (priv->store), &child_iter, 0, 
                                     ATTR_POINTER_COLUMN, attr,
                                     ATTR_NAME_COLUMN, priv->filter_string,
                                     ATTR_TYPE_COLUMN, DEFAULT_NEW_ATTRIBUTE_TYPE , -1);

  gtk_tree_model_filter_convert_child_iter_to_iter (GTK_TREE_MODEL_FILTER (priv->filter),
                                                    &filter_iter, &child_iter);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (priv->treeview), 2);

  filter_path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->filter), &filter_iter);
  gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->treeview), filter_path, col, TRUE);
  gtk_widget_activate (GTK_WIDGET (priv->treeview));

  gtk_tree_path_free (filter_path);
}


static void
contacts_attributes_editor_init (ContactsAttributesEditor *self)
{
  ContactsAttributesEditorPriv *priv = GET_PRIVATE (self);
  GtkVBox *vb = GTK_VBOX (self);
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *treeview_column;

  priv->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (priv->frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vb), priv->frame, FALSE, FALSE, 0);

  priv->treeview = gtk_tree_view_new ();
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->treeview), FALSE);
  gtk_container_add (GTK_CONTAINER (priv->frame), priv->treeview);

  /* delete option column */
  renderer = koto_cell_renderer_pixbuf_new ();
  g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_DELETE, NULL);

  g_signal_connect (G_OBJECT (renderer), "activated", G_CALLBACK (delete_renderer_activated_cb), self);

  treeview_column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
  g_object_set (G_OBJECT (treeview_column), "visible", FALSE, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (priv->treeview), treeview_column, TREE_DEL_COLUMN);

  /* type option column */
  GtkListStore *liststore;

  /* FIXME: this should be translatable somehow */
  liststore = gtk_list_store_new (1, G_TYPE_STRING);
  gtk_list_store_insert_with_values (liststore, NULL, 0, 0, "Work", -1);
  gtk_list_store_insert_with_values (liststore, NULL, 0, 0, "Home", -1);
  gtk_list_store_insert_with_values (liststore, NULL, 0, 0, "Other", -1);
  gtk_list_store_insert_with_values (liststore, NULL, 0, 0, "Pref", -1);

  renderer = gtk_cell_renderer_combo_new();
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (type_renderer_edited_cb), self);
  g_object_set (G_OBJECT (renderer), "model", liststore, "text-column", 0, "has-entry", FALSE, NULL);
  treeview_column = gtk_tree_view_column_new_with_attributes ("", renderer, "text", ATTR_TYPE_COLUMN, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (priv->treeview), treeview_column, TREE_TYPE_COLUMN);

  /* icon column */
  renderer = gtk_cell_renderer_pixbuf_new();
  g_object_set (G_OBJECT (renderer), "stock-id", priv->stock_id, NULL);
  treeview_column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (treeview_column, renderer, FALSE);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (priv->treeview), treeview_column, TREE_ICON_COLUMN);
  priv->icon_renderer = renderer;

  /* value column */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (value_renderer_edited_cb), self);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (priv->treeview),
      gtk_tree_view_column_new_with_attributes ("Value", renderer, "text", ATTR_VALUE_COLUMN, NULL), TREE_VALUE_COLUMN);


  /* add attribute button */
  priv->button = gtk_button_new_with_label ("Add Phone Number");
  gtk_widget_set_name (priv->button, "contacts-add-detail-button");
  g_signal_connect (G_OBJECT (priv->button), "clicked", G_CALLBACK (add_new_attribute), self);
  g_object_set (G_OBJECT (priv->button), "no-show-all", TRUE, NULL);
  gtk_box_pack_start (GTK_BOX (vb), priv->button, FALSE, FALSE, 0);

}

static void
contacts_attributes_editor_finalize (GObject *object)
{
  G_OBJECT_CLASS (contacts_attributes_editor_parent_class)->finalize (object);
}


static void
contacts_attributes_editor_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  ContactsAttributesEditorPriv *priv = GET_PRIVATE (object);

  switch (property_id)
  {
    case PROP_ATTR_LIST:
      g_value_set_object (value, priv->store);
      break;
    case PROP_FILTER:
      g_value_set_object (value, priv->filter_string);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
contacts_attributes_editor_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_ATTR_LIST:
      contacts_attributes_editor_set_attribute_store (CONTACTS_ATTRIBUTES_EDITOR (object),
                                           g_value_get_object (value));
      break;
    case PROP_FILTER:
      contacts_attributes_editor_set_type (CONTACTS_ATTRIBUTES_EDITOR (object),
                                             g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
contacts_attributes_editor_class_init (ContactsAttributesEditorClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  /* GtkVBoxClass* parent_class = GTK_VBOX_CLASS (klass); */
  GParamSpec *ps;

  object_class->finalize = contacts_attributes_editor_finalize;
  object_class->get_property = contacts_attributes_editor_get_property;
  object_class->set_property = contacts_attributes_editor_set_property;
  
  ps = g_param_spec_object ("attribute-store",
                            "attribute-store", NULL,
                            CONTACTS_TYPE_ATTRIBUTE_STORE,
                            G_PARAM_READWRITE |
                            G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK);
  g_object_class_install_property (object_class, PROP_ATTR_LIST, ps);
  
  ps = g_param_spec_string ("type", "type", NULL, "",
                            G_PARAM_READWRITE |
                            G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK);
  g_object_class_install_property (object_class, PROP_FILTER, ps);
  
  g_type_class_add_private (klass, sizeof (ContactsAttributesEditorPriv));

}

GtkWidget *
contacts_attributes_editor_new (ContactsAttributeStore *store, const gchar *type)
{
  g_return_val_if_fail (CONTACTS_IS_ATTRIBUTE_STORE (store), NULL);

  return g_object_new (CONTACTS_TYPE_ATTRIBUTES_EDITOR,
                       "attribute-store", store,
                       "type", type,
                       NULL);
}


void
contacts_attributes_editor_set_attribute_store (ContactsAttributesEditor *editor,
                                     ContactsAttributeStore *store)
{
  ContactsAttributesEditorPriv *priv;

  g_return_if_fail (CONTACTS_IS_ATTRIBUTES_EDITOR (editor));
  g_return_if_fail (CONTACTS_IS_ATTRIBUTE_STORE (store));
  
  priv = GET_PRIVATE (editor);
  
  priv->store = GTK_TREE_MODEL (store);
  priv->filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL);
  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview), priv->store);
  
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filter),
      (GtkTreeModelFilterVisibleFunc) filter_visible_func, priv, NULL);
  g_signal_connect_swapped (priv->filter, "row-inserted", G_CALLBACK (update_visible_treeviews), editor);
  g_signal_connect_swapped (priv->filter, "row-deleted", G_CALLBACK (update_visible_treeviews), editor);
  update_visible_treeviews (editor);
}

void
contacts_attributes_editor_set_type (ContactsAttributesEditor *editor, const gchar *type)
{
  ContactsAttributesEditorPriv *priv;

  g_return_if_fail (CONTACTS_IS_ATTRIBUTES_EDITOR (editor));
  
  priv = GET_PRIVATE (editor);
  
  if (priv->filter_string)
    g_free (priv->filter_string);
  
  priv->filter_string = g_strdup (type);
  
  if (g_str_equal (type, EVC_TEL))
    g_object_set (priv->icon_renderer, "stock-id", MOKO_STOCK_CONTACT_PHONE, NULL);
  else if (g_str_equal (type, EVC_EMAIL))
    g_object_set (priv->icon_renderer, "stock-id", MOKO_STOCK_CONTACT_EMAIL, NULL);
  else
    g_object_set (priv->icon_renderer, "stock-id", "", NULL);
  
  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview), priv->filter);
  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
}

void
contacts_attributes_editor_set_editable (ContactsAttributesEditor *editor, gboolean editable)
{
  ContactsAttributesEditorPriv *priv;
  GtkTreeViewColumn *col;
  GList *list = NULL;
  GtkTreeSelection *sel;

  g_return_if_fail (CONTACTS_IS_ATTRIBUTES_EDITOR (editor));
  
  priv = GET_PRIVATE (editor);
  
  g_return_if_fail (GTK_IS_TREE_VIEW (priv->treeview));
  
  g_object_set (G_OBJECT (priv->treeview), "can-focus", editable, NULL);
  col = gtk_tree_view_get_column (GTK_TREE_VIEW (priv->treeview), TREE_DEL_COLUMN);
  g_object_set (G_OBJECT (col), "visible", editable, NULL);
  
  
  col = gtk_tree_view_get_column (GTK_TREE_VIEW (priv->treeview), TREE_TYPE_COLUMN);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editable, NULL);
  g_list_free (list);
  
  col = gtk_tree_view_get_column (GTK_TREE_VIEW (priv->treeview), TREE_VALUE_COLUMN);
  list = gtk_tree_view_column_get_cell_renderers (col);
  g_object_set (G_OBJECT (list->data), "editable", editable, NULL);
  g_list_free (list);

  g_object_set (G_OBJECT (priv->button), "visible", editable, NULL);

  update_visible_treeviews (editor);

  /* ensure selection is possible in edit mode - cell editing is not possible
   * without it
   */
  if (editable)
  {
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  }
  else
  {
    /* disable selection when not in editing mode */
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_NONE);
  }
}
