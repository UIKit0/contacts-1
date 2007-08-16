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
static void search_toggle_cb (GtkWidget *button, ContactsData *data);
static void dial_contact_clicked_cb (GtkWidget *button, ContactsData *data);
static void new_contact_clicked_cb (GtkWidget *button, ContactsData *data);
static void on_entry_changed (GtkEntry *entry, HitoContactModelFilter *filter);
static void on_selection_changed (GtkTreeSelection *selection, ContactsData *data);


/* ui creation */
void
create_contacts_list_page (ContactsData *data)
{
  GtkWidget *toolbar, *box, *hbox, *scrolled, *treeview;
  GtkToolItem *toolitem;
  GtkWidget *search_toggle;
  GtkTreeModel *group_store, *contact_store, *contact_filter;

  group_store = hito_group_store_new ();
  hito_group_store_set_view (HITO_GROUP_STORE (group_store), data->view);
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_all_group_new ());
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_separator_group_new (-99));
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_separator_group_new (99));
  hito_group_store_add_group (HITO_GROUP_STORE (group_store), hito_no_category_group_new ());

  contact_store = hito_contact_store_new (data->view);
  contact_filter = hito_contact_model_filter_new (HITO_CONTACT_STORE (contact_store));

  box = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (data->notebook), box, gtk_image_new_from_stock (GTK_STOCK_INDEX, GTK_ICON_SIZE_BUTTON));
  gtk_container_child_set (GTK_CONTAINER (data->notebook), box, "tab-expand", TRUE, "tab-fill", TRUE, NULL);

  /* toolbar */
  toolbar = gtk_toolbar_new ();
  gtk_box_pack_start (GTK_BOX (box), toolbar, FALSE, FALSE, 0);

  toolitem = gtk_tool_button_new_from_stock (MOKO_STOCK_CALL_DIAL);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 0);
  g_signal_connect (toolitem, "clicked", 
                    G_CALLBACK (dial_contact_clicked_cb), data);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), 1);

  toolitem = gtk_tool_button_new_from_stock (MOKO_STOCK_SMS_NEW);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 2);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), 3);

  toolitem = gtk_tool_button_new_from_stock (GTK_STOCK_NEW);
  g_signal_connect (toolitem, "clicked", G_CALLBACK (new_contact_clicked_cb), data);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, 4);

  /* search/filter bar */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

  search_toggle = gtk_toggle_button_new ();
  gtk_widget_set_name (search_toggle, "mokosearchbutton");
  gtk_button_set_image (GTK_BUTTON (search_toggle), gtk_image_new_from_stock (GTK_STOCK_FIND, GTK_ICON_SIZE_SMALL_TOOLBAR));
  g_signal_connect (G_OBJECT (search_toggle), "toggled",  (GCallback) search_toggle_cb, data);
  gtk_box_pack_start (GTK_BOX (hbox), search_toggle, FALSE, FALSE, 0);

  data->search_entry = gtk_entry_new ();
  gtk_widget_set_name (data->search_entry, "mokosearchentry");
  g_signal_connect (data->search_entry, "changed", G_CALLBACK (on_entry_changed), contact_filter);
  g_object_set (G_OBJECT (data->search_entry), "no-show-all", TRUE, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), data->search_entry, TRUE, TRUE, 0);

  data->groups_combo = hito_group_combo_new (HITO_GROUP_STORE (group_store));
  hito_group_combo_connect_filter (HITO_GROUP_COMBO (data->groups_combo),
                                   HITO_CONTACT_MODEL_FILTER (contact_filter));
  gtk_combo_box_set_active (GTK_COMBO_BOX (data->groups_combo), 0);
  gtk_box_pack_start (GTK_BOX (hbox), data->groups_combo, TRUE, TRUE, 0);


  /* main treeview */
  scrolled = moko_finger_scroll_new ();
  gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);

  treeview = hito_contact_view_new (HITO_CONTACT_STORE (contact_store),
                                    HITO_CONTACT_MODEL_FILTER (contact_filter));
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)),
                    "changed", G_CALLBACK (on_selection_changed), data);
  gtk_container_add (GTK_CONTAINER (scrolled), treeview);

}

/* callbacks */

static void
on_entry_changed (GtkEntry *entry, HitoContactModelFilter *filter)
{
  hito_contact_model_filter_set_text (filter, gtk_entry_get_text (entry));
}

static void
on_selection_changed (GtkTreeSelection *selection, ContactsData *data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  EContact *contact;

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COLUMN_CONTACT, &contact, -1);
    contacts_details_page_set_contact (data, contact);
    contacts_history_page_set_contact (data, contact);
    contacts_groups_page_set_contact (data, contact);
    g_object_unref (contact);
  } else {
    contacts_details_page_set_contact (data, NULL);
    contacts_history_page_set_contact (data, NULL);
    contacts_groups_page_set_contact (data, NULL);
  }
}

static void
search_toggle_cb (GtkWidget *button, ContactsData *data)
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
new_contact_clicked_cb (GtkWidget *button, ContactsData *data)
{
  /* NULL is used to indicate "new contact" */
  contacts_details_page_set_contact (data, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (data->notebook), DETAIL_PAGE_NUM);
  contacts_details_page_set_editable (data, TRUE);
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
show_contact_numbers (const gchar *name, GList *numbers, ContactsData *data)
{
  GList *n;
  gint num_tels = 0;

  num_tels = g_list_length (numbers);

  if (num_tels < 1)
    return;
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

static void
dial_contact_clicked_cb (GtkWidget *button, ContactsData *data)
{
  EContact *contact = NULL;
  GList *attributes, *a, *numbers = NULL;

  contact = g_object_get_data (G_OBJECT (data->groups), "contact");

  if (!E_IS_CONTACT (contact))
    return;

  attributes = e_vcard_get_attributes (E_VCARD (contact));
  for (a = attributes; a; a = g_list_next (a))
  {
    const gchar *name, *value, *type;
    name = e_vcard_attribute_get_name (a->data);
    value = hito_vcard_attribute_get_value_string (a->data);
    type = hito_vcard_attribute_get_type (a->data);

    if (!strcmp (name, EVC_TEL))
    {
      numbers = g_list_append (numbers, a->data);
      continue;
    }
  }
  show_contact_numbers ("hello", numbers, data);
  g_list_free (numbers);
}

