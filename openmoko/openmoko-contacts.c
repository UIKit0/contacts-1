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
#include "hito-contact-store.h"
#include "hito-contact-model-filter.h"
#include "hito-contact-view.h"
#include "hito-contact-preview.h"
#include "hito-group-store.h"
#include "hito-group-combo.h"
#include "hito-all-group.h"
#include "hito-category-group.h"
#include "hito-no-category-group.h"
#include "hito-separator-group.h"


static EBook *book;
static EBookView *view = NULL;

typedef struct
{
  GtkWidget *search_entry;
  GtkWidget *groups_combo;
} ContactsData;

static void
on_entry_changed (GtkEntry *entry, HitoContactModelFilter *filter)
{
  hito_contact_model_filter_set_text (filter, gtk_entry_get_text (entry));
}

static void
on_selection_changed (GtkTreeSelection *selection, HitoContactPreview *preview)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  EContact *contact;

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COLUMN_CONTACT, &contact, -1);
    hito_contact_preview_set_contact (preview, contact);
    g_object_unref (contact);
  } else {
    hito_contact_preview_set_contact (preview, NULL);
  }
}

static void
toggle_search_groups (GtkWidget *button, ContactsData *data)
{
  gboolean search;
  
  search = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  if (search)
  {
    gtk_widget_show (data->search_entry);
    gtk_widget_hide (data->groups_combo);
  }
  else
  {
    gtk_widget_show (data->groups_combo);
    gtk_widget_hide (data->search_entry);
  }
}

static void
make_contact_view (ContactsData *data)
{
  GtkWidget *window;
  GtkWidget *toolbar, *box, *hbox, *scrolled, *treeview, *preview, *notebook;
  GtkToolItem *toolitem;
  GtkWidget *entry, *search_toggle;
  GtkTreeModel *group_store, *contact_store, *contact_filter;

  group_store = hito_group_store_new ();
  hito_group_store_set_view (HITO_GROUP_STORE (group_store), view);
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_all_group_new ());
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_separator_group_new (-99));
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_separator_group_new (99));
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_no_category_group_new ());
  
  contact_store = hito_contact_store_new (view);
  contact_filter = hito_contact_model_filter_new (HITO_CONTACT_STORE (contact_store));

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
  gtk_window_set_title (GTK_WINDOW (window), "Contacts");
  


  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_BOTTOM);
  gtk_container_add (GTK_CONTAINER (window), notebook);

  box = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, gtk_image_new_from_stock (GTK_STOCK_INDEX, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_container_child_set (GTK_CONTAINER (notebook), box, "tab-expand", TRUE, "tab-fill", TRUE, NULL);

  /* index toolbar */
  toolbar = gtk_toolbar_new ();
  gtk_box_pack_start (GTK_BOX (box), toolbar, FALSE, FALSE, 0);

  toolitem = gtk_tool_button_new_from_stock (GTK_STOCK_NEW);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 0);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), 0);

  toolitem = gtk_tool_button_new_from_stock (GTK_STOCK_ABOUT);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 0);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), 0);

  toolitem = gtk_tool_button_new_from_stock (GTK_STOCK_ABOUT);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 0);


  /* search/filter bar */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

  search_toggle = gtk_toggle_button_new ();
  gtk_button_set_image (GTK_BUTTON (search_toggle), gtk_image_new_from_stock (GTK_STOCK_FIND, GTK_ICON_SIZE_SMALL_TOOLBAR));
  g_signal_connect (G_OBJECT (search_toggle), "toggled",  (GCallback) toggle_search_groups, data);
  gtk_box_pack_start (GTK_BOX (hbox), search_toggle, FALSE, FALSE, 0);

  data->search_entry = gtk_entry_new ();
  g_signal_connect (data->search_entry, "changed", G_CALLBACK (on_entry_changed), contact_filter);
  g_object_set (G_OBJECT (data->search_entry), "no-show-all", TRUE, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), data->search_entry, TRUE, TRUE, 0);

  data->groups_combo = hito_group_combo_new (HITO_GROUP_STORE (group_store));
  hito_group_combo_connect_filter (HITO_GROUP_COMBO (data->groups_combo),
                                   HITO_CONTACT_MODEL_FILTER (contact_filter));
  gtk_combo_box_set_active (GTK_COMBO_BOX (data->groups_combo), 0);
  gtk_box_pack_start (GTK_BOX (hbox), data->groups_combo, TRUE, TRUE, 0);


  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);

  treeview = hito_contact_view_new (HITO_CONTACT_STORE (contact_store),
                                    HITO_CONTACT_MODEL_FILTER (contact_filter));
  gtk_container_add (GTK_CONTAINER (scrolled), treeview);

  /* view/edit */

  box = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, gtk_image_new_from_stock (GTK_STOCK_INFO, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_container_child_set (GTK_CONTAINER (notebook), box, "tab-expand", TRUE, "tab-fill", TRUE, NULL);

  toolbar = gtk_toolbar_new ();
  gtk_box_pack_start (GTK_BOX (box), toolbar, FALSE, FALSE, 0);

  toolitem = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_EDIT);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 0);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), 1);

  toolitem = gtk_tool_button_new_from_stock (GTK_STOCK_DELETE);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 2);

  preview = hito_contact_preview_new ();
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)),
                    "changed", G_CALLBACK (on_selection_changed), preview);

  gtk_box_pack_start (GTK_BOX (box), preview, FALSE, FALSE, 0);


  /* history */
  box = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, gtk_image_new_from_stock (GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_container_child_set (GTK_CONTAINER (notebook), box, "tab-expand", TRUE, "tab-fill", TRUE, NULL);

  /* groups */
  box = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, gtk_image_new_from_stock (GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_container_child_set (GTK_CONTAINER (notebook), box, "tab-expand", TRUE, "tab-fill", TRUE, NULL);

  gtk_widget_show_all (window);
}

int
main (int argc, char **argv)
{
  EBookQuery *query;
  ContactsData *data;

  data = g_new (ContactsData, 1);

  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  book = e_book_new_system_addressbook (NULL);
  g_assert (book);

  e_book_open (book, FALSE, NULL);

  query = e_book_query_any_field_contains (NULL);

  e_book_get_book_view (book, query, NULL, 0, &view, NULL);
  g_assert (view);
  e_book_query_unref (query);
  
  e_book_view_start (view);

  make_contact_view (data);
  
  gtk_main ();

  e_book_view_stop (view);
  g_object_unref (book);

  g_free (data);
  
  return 0;
}
