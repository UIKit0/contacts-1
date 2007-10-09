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
#include <moko-search-bar.h>

#include "openmoko-contacts-list.h"
#include "openmoko-contacts-details.h"
#include "openmoko-contacts-history.h"
#include "openmoko-contacts-groups.h"
#include "openmoko-contacts-util.h"

#include "hito-contact-store.h"
#include "hito-contact-model-filter.h"
#include "hito-contact-view.h"
#include "hito-group-store.h"
#include "hito-group-combo.h"
#include "hito-all-group.h"
#include "hito-category-group.h"
#include "hito-no-category-group.h"
#include "hito-separator-group.h"
#include "hito-vcard-util.h"


/* callbacks */
static void dial_contact_clicked_cb (GtkWidget *button, ContactsData *data);
static void new_contact_clicked_cb (GtkWidget *button, ContactsData *data);
static void on_entry_changed (MokoSearchBar *bar, GtkEntry *entry, HitoContactModelFilter *filter);
static void searchbar_toggled_cb (MokoSearchBar *searchbar, gboolean foo, ContactsData *data);
static void on_selection_changed (GtkTreeSelection *selection, ContactsData *data);
static void sequence_complete_cb (EBookView *view, gchar *arg1, ContactsData *data);
static void rows_reordererd_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer arg3, ContactsData *data);

/* ui creation */
void
create_contacts_list_page (ContactsData *data)
{
  GtkWidget *toolbar, *box, *scrolled, *searchbar;
  GtkTreeModel *group_store, *contact_store, *contact_filter;
  GtkToolItem *toolitem;

  group_store = hito_group_store_new ();
  hito_group_store_set_view (HITO_GROUP_STORE (group_store), data->view);
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_all_group_new ());
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_separator_group_new (-99));
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_separator_group_new (99));
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_no_category_group_new ());

  data->contacts_store = contact_store = hito_contact_store_new (data->view);
  contact_filter = hito_contact_model_filter_new (HITO_CONTACT_STORE (contact_store));
  g_signal_connect (contact_store, "rows-reordered", G_CALLBACK (rows_reordererd_cb), data);

  box = gtk_vbox_new (FALSE, 0);
  contacts_notebook_add_page_with_icon (data->notebook, box, GTK_STOCK_INDEX);

  /* toolbar */
  toolbar = gtk_toolbar_new ();
  gtk_box_pack_start (GTK_BOX (box), toolbar, FALSE, FALSE, 0);

  data->dial_button = gtk_tool_button_new_from_stock (MOKO_STOCK_CALL_DIAL);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (data->dial_button), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), data->dial_button, 0);
  g_signal_connect (data->dial_button, "clicked", 
                    G_CALLBACK (dial_contact_clicked_cb), data);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), 1);

  data->sms_button = gtk_tool_button_new_from_stock (MOKO_STOCK_SMS_NEW);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (data->sms_button), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), data->sms_button, 2);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), 3);

  toolitem = gtk_tool_button_new_from_stock (MOKO_STOCK_CONTACT_NEW);
  g_signal_connect (toolitem, "clicked", G_CALLBACK (new_contact_clicked_cb), data);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 4);

  /* search/filter bar */

  /* we use a special combo box from libhito */
  data->groups_combo = hito_group_combo_new (HITO_GROUP_STORE (group_store));
  hito_group_combo_connect_filter (HITO_GROUP_COMBO (data->groups_combo),
                                   HITO_CONTACT_MODEL_FILTER (contact_filter));
  gtk_combo_box_set_active (GTK_COMBO_BOX (data->groups_combo), 0);

  /* create the searchbar with the above combobox */
  searchbar = moko_search_bar_new_with_combo (GTK_COMBO_BOX (data->groups_combo));
  gtk_box_pack_start (GTK_BOX (box), searchbar, FALSE, FALSE, 0);

  g_signal_connect (searchbar, "text-changed", G_CALLBACK (on_entry_changed), contact_filter);
  g_signal_connect (searchbar, "toggled", G_CALLBACK (searchbar_toggled_cb), data);

  /* main treeview */
  scrolled = moko_finger_scroll_new ();
  gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);

  data->contacts_treeview = hito_contact_view_new (HITO_CONTACT_STORE (data->contact_store),
                                    HITO_CONTACT_MODEL_FILTER (contact_filter));
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->contacts_treeview)),
                    "changed", G_CALLBACK (on_selection_changed), data);
  gtk_container_add (GTK_CONTAINER (scrolled), data->contacts_treeview);

  g_signal_connect (data->view, "sequence-complete", G_CALLBACK (sequence_complete_cb), data);

}

/* callbacks */

static void
searchbar_toggled_cb (MokoSearchBar *searchbar, gboolean foo, ContactsData *data)
{
  GtkTreeModelFilter *filter;
  GtkEntry *entry;
  GtkComboBox *combo;
  filter = GTK_TREE_MODEL_FILTER (
      gtk_tree_view_get_model (GTK_TREE_VIEW (data->contacts_treeview)));

  entry = moko_search_bar_get_entry (searchbar);
  combo = moko_search_bar_get_combo_box (searchbar);

  if (moko_search_bar_search_visible (searchbar))
  {
    hito_contact_model_filter_set_text (HITO_CONTACT_MODEL_FILTER (filter),
       gtk_entry_get_text (entry));
    gtk_combo_box_set_active (combo, 0);
  }
  else
  {
    hito_contact_model_filter_set_text (HITO_CONTACT_MODEL_FILTER (filter), NULL);
    gtk_combo_box_set_active (combo, 0);
  }

}


static void
sequence_complete_cb (EBookView *view, gchar *arg1, ContactsData *data)
{
  GtkTreeSelection *selection;
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->contacts_treeview));

  /* try to make sure at least one contact is selected */
  if (gtk_tree_selection_count_selected_rows (selection) == 0)
  {
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreePath *path;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->contacts_treeview));
    if (!gtk_tree_model_get_iter_first (model, &iter))
      return;
    path = gtk_tree_model_get_path (model, &iter);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (data->contacts_treeview), path, NULL, FALSE);
    gtk_tree_path_free (path);
  }

}

static void
on_entry_changed (MokoSearchBar *bar, GtkEntry *entry, HitoContactModelFilter *filter)
{
  hito_contact_model_filter_set_text (filter, gtk_entry_get_text (entry));
}

static void
on_selection_changed (GtkTreeSelection *selection, ContactsData *data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  EContact *contact = NULL;
  GList *numbers;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
  {
    gtk_tree_model_get (model, &iter, COLUMN_CONTACT, &contact, -1);
  }

  contacts_set_current_contact (data, contact);

  if (contact)
    numbers = hito_vcard_get_named_attributes (E_VCARD (contact), EVC_TEL);
  else
    numbers = NULL;

  gtk_widget_set_sensitive (GTK_WIDGET (data->sms_button), numbers ? TRUE : FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (data->dial_button), numbers ? TRUE : FALSE);

  g_list_free (numbers);


}

static void
ebook_id_callback(EBook *book, EBookStatus status, const char *id, ContactsData *data)
{
  EContact *contact;
  e_book_get_contact (data->book, id, &contact, NULL);
  contacts_set_current_contact (data, contact);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (data->notebook), DETAIL_PAGE_NUM);
  contacts_details_page_set_editable (data, TRUE);
}

static void
new_contact_clicked_cb (GtkWidget *button, ContactsData *data)
{
  EContact *contact;
  contact = e_contact_new ();
  e_book_async_add_contact (data->book, contact, (EBookIdCallback) ebook_id_callback, data);
}

static void
on_dial_number_clicked (GtkWidget *eb, GdkEventButton *event, GtkDialog *dialog)
{
  EVCardAttribute *att;
  const gchar *number;

  att = g_object_get_data (G_OBJECT (eb), "contact");
  number = hito_vcard_attribute_get_value_string (att);
  openmoko_contacts_util_dial_number (number);

  gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);
}


static void
rows_reordererd_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer arg3, ContactsData *data)
{
  GtkTreePath *selected_path;
  GtkTreeIter selected_iter;
  GtkTreeSelection *selection;
  GtkTreeModel *treemodel;


  /* this makes sure the selected row is always visible */

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->contacts_treeview));
  gtk_tree_selection_get_selected (selection, &treemodel, &selected_iter);
  selected_path = gtk_tree_model_get_path (treemodel, &selected_iter);

  gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (data->contacts_treeview), selected_path, NULL, FALSE, 0, 0);

  gtk_tree_path_free (selected_path);
}


static void
show_contact_numbers (const gchar *name, GList *numbers, ContactsData *data)
{
  GList *n;
  gint num_tels = 0;

  num_tels = g_list_length (numbers);

  if (num_tels < 1)
  {
    g_print ("Dial: This contact does not have any numbers\n");
    return;
  }
  else if (num_tels == 1)
  {
    /* dial */
    openmoko_contacts_util_dial_number (
            hito_vcard_attribute_get_value_string (numbers->data));
  }
  else
  {
    /* Make a dialog with a list of numbers, which are one-click dialling */
    /* dial on click, and then close the window */
    GtkWidget *dialog, *vbox, *hbox, *image, *label;

    dialog = gtk_dialog_new_with_buttons ("Please choose a number to call",
                                          GTK_WINDOW (data->window),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 12);                                    
    vbox = gtk_vbox_new (FALSE, 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox), vbox, 
                        FALSE, FALSE, 12);

    for (n = numbers; n; n = n->next)
    {
      GtkWidget *button = gtk_event_box_new ();
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 8);

      hbox = gtk_hbox_new (FALSE, 6);
      gtk_container_add (GTK_CONTAINER (button), hbox);
      
      image = gtk_image_new_from_stock (MOKO_STOCK_CONTACT_PHONE, 
                                        GTK_ICON_SIZE_BUTTON);
      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

      label = gtk_label_new (hito_vcard_attribute_get_value_string (n->data));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

      g_signal_connect (button, "button-release-event",
                        G_CALLBACK (on_dial_number_clicked), (gpointer)dialog);
      g_object_set_data (G_OBJECT (button), "contact", n->data);
    }
    
    /*g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy), dialog);*/
    gtk_widget_show_all (dialog);
    gint res = gtk_dialog_run (GTK_DIALOG (dialog));
    res++;
    gtk_widget_destroy (dialog);
  } 
}



EContact*
contacts_list_get_selected_contact (ContactsData *data)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  EContact *contact = NULL;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->contacts_treeview));

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    gtk_tree_model_get (model, &iter, COLUMN_CONTACT, &contact, -1);

  return contact;
}


static void
dial_contact_clicked_cb (GtkWidget *button, ContactsData *data)
{
  EContact *contact = NULL;
  GList *numbers = NULL;

  contact = contacts_list_get_selected_contact (data);

  if (!E_IS_CONTACT (contact))
  {
    g_print ("Dial: This is not a valid contact\n");
    return;
  }

  numbers = hito_vcard_get_named_attributes (E_VCARD (contact), EVC_TEL);
  show_contact_numbers ("hello", numbers, data);
  g_list_free (numbers);
}

