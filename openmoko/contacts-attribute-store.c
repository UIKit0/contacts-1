/*
 * hito
 * Copyright (C) OpenedHand Ltd 2007 <info@openedhand.com>
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

#include "contacts-attribute-store.h"
#include "hito-vcard-util.h"

G_DEFINE_TYPE (ContactsAttributeStore, contacts_attribute_store, GTK_TYPE_LIST_STORE);

typedef struct _ContactAttributeStorePriv ContactsAttributeStorePriv;

struct _ContactAttributeStorePriv
{
  EVCard *vcard;
  gboolean updating;
};

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CONTACTS_TYPE_ATTRIBUTE_STORE, ContactsAttributeStorePriv))

static void free_liststore_data (GtkTreeModel *model, GtkTreePath *path,
                                 GtkTreeIter *iter, gpointer data);


static void
attribute_store_row_changed_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, ContactsAttributeStore *self)
{
  ContactsAttributeStorePriv *priv = GET_PRIVATE (self);
  EVCardAttribute *attr;
  gchar *value;

  if (priv->updating)
    return;

  gtk_tree_model_get (model, iter,
                      ATTR_POINTER_COLUMN, &attr,
                      ATTR_VALUE_COLUMN, &value,
                      -1);

  /* remove old values and add new one */
  e_vcard_attribute_remove_values (attr);

  /* TODO: check for multi valued string */
  e_vcard_attribute_add_value (attr, value);

  gtk_tree_model_get (model, iter, ATTR_TYPE_COLUMN, &value, -1);
  hito_vcard_attribute_set_type (attr, value);
}

static void
attribute_store_row_inserted_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, ContactsAttributeStore *self)
{
  EVCardAttribute *attr;
  ContactsAttributeStorePriv *priv = GET_PRIVATE (self);

  if (priv->updating)
    return;

  gtk_tree_model_get (model, iter, ATTR_POINTER_COLUMN, &attr, -1);
  e_vcard_add_attribute (priv->vcard, attr);
}

static void
contacts_attribute_store_init (ContactsAttributeStore *self)
{
  
  GType col_types[] = {
    G_TYPE_POINTER,
    G_TYPE_STRING,
    G_TYPE_STRING,
    G_TYPE_STRING
  };
  
  gtk_list_store_set_column_types (GTK_LIST_STORE (self), 
                                   G_N_ELEMENTS (col_types), col_types);

  g_signal_connect (G_OBJECT (self), "row-changed",
                    G_CALLBACK (attribute_store_row_changed_cb), self);
  g_signal_connect (G_OBJECT (self), "row-inserted",
                    G_CALLBACK (attribute_store_row_inserted_cb), self);
}

static void
contacts_attribute_store_finalize (GObject *self)
{
  ContactsAttributeStorePriv *priv;

  /* deinitalization */
 
  g_return_if_fail (CONTACTS_IS_ATTRIBUTE_STORE (self));
  priv = GET_PRIVATE (self);

  if (priv->vcard)
  {
    g_object_unref (priv->vcard);
  }
  gtk_tree_model_foreach (GTK_TREE_MODEL (self), (GtkTreeModelForeachFunc) free_liststore_data, NULL);

  G_OBJECT_CLASS (contacts_attribute_store_parent_class)->finalize (self);
}

static void
contacts_attribute_store_class_init (ContactsAttributeStoreClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  /* GtkListStoreClass* parent_class = GTK_LIST_STORE_CLASS (klass); */

  g_type_class_add_private (klass, sizeof (ContactsAttributeStorePriv));

  object_class->finalize = contacts_attribute_store_finalize;
}

static void
free_liststore_data (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  gchar *value = NULL, *type = NULL;
  gtk_tree_model_get (model, iter, ATTR_VALUE_COLUMN, &value, ATTR_TYPE_COLUMN, &type, -1);
  g_free (value);
  g_free (type);
}


/*** Public Functions ***/

GtkTreeModel *
contacts_attribute_store_new ()
{
  return g_object_new (CONTACTS_TYPE_ATTRIBUTE_STORE, NULL);
}


void
contacts_attribute_store_set_vcard (ContactsAttributeStore *store, EVCard *card)
{
  GList *attributes, *a;
  ContactsAttributeStorePriv *priv;
  priv = GET_PRIVATE (store);
  
  g_return_if_fail (CONTACTS_IS_ATTRIBUTE_STORE (store));

  /* we allow for NULL to be set */
  if (card)
    g_return_if_fail (E_IS_VCARD (card));

  if (priv->vcard == card)
    return;

  priv->updating = TRUE;

  if (priv->vcard)
  {
    g_object_unref (priv->vcard);
  }
  priv->vcard = card;

  if (card)
    g_object_ref (card);

  /* clear the current attributes */
  gtk_tree_model_foreach (GTK_TREE_MODEL (store), (GtkTreeModelForeachFunc) free_liststore_data, NULL);
  gtk_list_store_clear (GTK_LIST_STORE (store));

  if (card)
  {
    attributes = e_vcard_get_attributes (card);

    for (a = attributes; a; a = g_list_next (a))
    {
      const gchar *name, *value;

      name = e_vcard_attribute_get_name (a->data);
          
      /* we don't really want to put photo data in the attribute list */
      if (g_str_equal (name, EVC_PHOTO))
        continue;

      value = hito_vcard_attribute_get_value_string (a->data);

      gtk_list_store_insert_with_values (GTK_LIST_STORE (store), NULL, 0,
          ATTR_POINTER_COLUMN, a->data,
          ATTR_NAME_COLUMN, name,
          ATTR_TYPE_COLUMN, hito_vcard_attribute_get_type (a->data),
          ATTR_VALUE_COLUMN, value,
          -1);
    }
  }
  
  priv->updating = FALSE;

}

void
contacts_attribute_store_remove (ContactsAttributeStore *store, GtkTreeIter *iter)
{
  ContactsAttributeStorePriv *priv;
  EVCardAttribute *attr;

  g_return_if_fail (CONTACTS_IS_ATTRIBUTE_STORE (store));
  
  priv = GET_PRIVATE (store);
  
  /* remove attribute from contact */
  gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
                      ATTR_POINTER_COLUMN, &attr, -1);
  e_vcard_remove_attribute (priv->vcard, attr);

  /* remove attribute row from model */
  gtk_list_store_remove (GTK_LIST_STORE (store), iter);
}
