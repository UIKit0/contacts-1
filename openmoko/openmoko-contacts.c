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

#include "openmoko-contacts-list.h"
#include "openmoko-contacts-details.h"


static void
window_delete_event_cb (GtkWidget *widget, GdkEvent *event, ContactsData *data)
{

  free_contacts_details_page (data);
  gtk_main_quit ();

}

static void
make_contact_view (ContactsData *data)
{

  GtkWidget *box;

  data->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (data->window, "delete-event", G_CALLBACK (window_delete_event_cb), data);
  gtk_window_set_title (GTK_WINDOW (data->window), "Contacts");


  data->notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (data->notebook), GTK_POS_BOTTOM);
  gtk_container_add (GTK_CONTAINER (data->window), data->notebook);

  /* create notebook pages */
  create_contacts_list_page (data);
  create_contacts_details_page (data);


  /* history */
  box = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (data->notebook), box, gtk_image_new_from_stock (GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_container_child_set (GTK_CONTAINER (data->notebook), box, "tab-expand", TRUE, "tab-fill", TRUE, NULL);

  /* groups */
  box = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (data->notebook), box, gtk_image_new_from_stock (GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_container_child_set (GTK_CONTAINER (data->notebook), box, "tab-expand", TRUE, "tab-fill", TRUE, NULL);
  gtk_widget_show_all (data->window);
}

int
main (int argc, char **argv)
{
  EBookQuery *query;
  ContactsData *data;

  data = g_new (ContactsData, 1);

  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  data->book = e_book_new_system_addressbook (NULL);
  g_assert (data->book);

  e_book_open (data->book, FALSE, NULL);

  query = e_book_query_any_field_contains (NULL);

  e_book_get_book_view (data->book, query, NULL, 0, &data->view, NULL);
  g_assert (data->view);
  e_book_query_unref (query);
  
  e_book_view_start (data->view);

  make_contact_view (data);
  
  gtk_main ();

  e_book_view_stop (data->view);
  g_object_unref (data->book);

  g_free (data);
  
  return 0;
}
