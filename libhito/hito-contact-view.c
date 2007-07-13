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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libedataserver/e-data-server-util.h>
#include "hito-contact-view.h"

G_DEFINE_TYPE (HitoContactView, hito_contact_view, GTK_TYPE_TREE_VIEW);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), HITO_TYPE_CONTACT_VIEW, HitoContactViewPrivate))

typedef struct {
  HitoContactStore *store;
  HitoContactModelFilter *filter;
} HitoContactViewPrivate;

enum {
  PROP_0,
  PROP_BASE_MODEL,
  PROP_FILTER,
};

/*
 * Private methods.
 */

/*
 * Cell data function for the name column.
 */
static void
name_data_func (GtkTreeViewColumn *tree_column,
              GtkCellRenderer   *cell,
              GtkTreeModel      *model,
              GtkTreeIter       *iter,
              gpointer           user_data)
{
  EContact *contact = NULL;

  gtk_tree_model_get (model, iter, COLUMN_CONTACT, &contact, -1);
  if (!contact)
    return;

  /* Get the display name from the store, so that the display and sort names are in sync */
  g_object_set (cell, "text", e_contact_get_const (contact, E_CONTACT_FULL_NAME), NULL);

  g_object_unref (contact);
}

/*
 * Custom function for interactive searches.
 */
static gboolean
search_equal_func (GtkTreeModel *model, gint column,
                   const gchar *key, GtkTreeIter *iter, gpointer search_data)
{
  EContact *contact = NULL;
  const char *name;
  gboolean found;
  
  gtk_tree_model_get (model, iter, COLUMN_CONTACT, &contact, -1);

  if (!contact)
    return FALSE;

  name = e_contact_get_const (contact, E_CONTACT_FULL_NAME);

  found = e_util_utf8_strstrcasedecomp (name, key) != NULL;
  
  g_object_unref (contact);
  
  /* GtkTreeView is insane */
  return ! found;
}

/*
 * Object methods.
 */

static void
hito_contact_view_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    /* TODO */
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
hito_contact_view_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  HitoContactViewPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
  case PROP_FILTER:
    if (priv->filter) {
      g_object_unref (priv->filter);
    }
    priv->filter = HITO_CONTACT_MODEL_FILTER (g_value_dup_object (value));
    break;
  case PROP_BASE_MODEL:
    if (priv->store) {
      g_object_unref (priv->store);
    }
    priv->store = HITO_CONTACT_STORE (g_value_dup_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
hito_contact_view_dispose (GObject *object)
{
  HitoContactViewPrivate *priv = GET_PRIVATE (object);

  if (priv->filter) {
    g_object_unref (priv->filter);
    priv->filter = NULL;
  }

  if (priv->store) {
    g_object_unref (priv->store);
    priv->store = NULL;
  }

  if (G_OBJECT_CLASS (hito_contact_view_parent_class)->dispose)
    G_OBJECT_CLASS (hito_contact_view_parent_class)->dispose (object);
}

static void
hito_contact_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (hito_contact_view_parent_class)->finalize (object);
}

static void
hito_contact_view_class_init (HitoContactViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (HitoContactViewPrivate));

  object_class->get_property = hito_contact_view_get_property;
  object_class->set_property = hito_contact_view_set_property;
  object_class->dispose = hito_contact_view_dispose;
  object_class->finalize = hito_contact_view_finalize;

  g_object_class_install_property (object_class, PROP_BASE_MODEL,
                                   g_param_spec_object ("base-model", "Base model", NULL,
                                                        HITO_TYPE_CONTACT_STORE,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class, PROP_FILTER,
                                   g_param_spec_object ("filter", "Filter", NULL,
                                                        HITO_TYPE_CONTACT_MODEL_FILTER,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
}

static void
hito_contact_view_init (HitoContactView *self)
{
  GtkTreeView *treeview;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  treeview = GTK_TREE_VIEW (self);

  gtk_tree_view_set_rules_hint (treeview, TRUE);
  gtk_tree_view_set_headers_visible (treeview, FALSE);
  gtk_tree_view_set_enable_search (treeview, TRUE);
  gtk_tree_view_set_search_column (treeview, COLUMN_CONTACT);
  gtk_tree_view_set_search_equal_func (treeview, search_equal_func, NULL, NULL);
  
  /* Summary column */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,  NULL);
  g_object_set (renderer,
                "ellipsize", PANGO_ELLIPSIZE_END,
                NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
                                           name_data_func, treeview, NULL);
  gtk_tree_view_append_column (treeview, column);
}


/*
 * Public methods.
 */

GtkWidget *
hito_contact_view_new (HitoContactStore *store, HitoContactModelFilter *filter)
{
  return g_object_new (HITO_TYPE_CONTACT_VIEW,
                       "model", filter ? (GtkTreeModel*)filter : (GtkTreeModel*)store,
                       "base-model", store,
                       "filter", filter,
                       NULL);
}

#if 0
/**
 * hito_contact_view_get_selected_contact:
 * @view: A #HitoContactView.
 *
 * Return the currently selected #EContact, or #NULL if there is now row
 * selected.  When finished with the #EContact, unref it with g_object_unref().
 *
 * Returns: a #EContact with its reference count incremented, or #NULL.
 */
HitoContact *
hito_contact_view_get_selected_contact (HitoContactView *view)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  EContact *contact = NULL;
  
  g_return_val_if_fail (HITO_IS_CONTACT_VIEW (view), NULL);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
    return NULL;
  }
  
  gtk_tree_model_get (model, &iter, COLUMN_CONTACT, &contact, -1);

  return contact;
}
#endif

gboolean
hito_contact_view_get_selected_iter (HitoContactView *view, GtkTreeIter *iter)
{
  GtkTreeSelection *selection;
  
  g_return_val_if_fail (HITO_IS_CONTACT_VIEW (view), FALSE);
  g_return_val_if_fail (iter, FALSE);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  
  return gtk_tree_selection_get_selected (selection, NULL, iter);
}
  