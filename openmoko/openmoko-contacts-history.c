/*
 * Copyright (C) 2006-2007 by OpenMoko, Inc.
 * Written by OpenedHand Ltd <info@openedhand.com>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "openmoko-contacts.h"
#include "contacts-history.h"
#include <moko-stock.h>

static void on_moko_journal_entry_activated (ContactsHistory *history, MokoJournalEntry *entry);


void
create_contacts_history_page (ContactsData *data)
{

  data->history = contacts_history_new ();

  g_signal_connect (G_OBJECT (data->history), "entry-activated",
                    G_CALLBACK (on_moko_journal_entry_activated), NULL);

  gtk_notebook_append_page (
      GTK_NOTEBOOK (data->notebook),
      data->history,
      gtk_image_new_from_stock (MOKO_STOCK_HISTORY, GTK_ICON_SIZE_LARGE_TOOLBAR)
      );

  gtk_container_child_set (GTK_CONTAINER (data->notebook), data->history,
      "tab-expand", TRUE, "tab-fill", TRUE, NULL);

}

void
contacts_history_page_free (ContactsData *data)
{
}

void
contacts_history_page_set_contact (ContactsData *data, EContact *contact)
{
  const gchar *uid = NULL;

  /* Get the contacts uid and update the history widget */
  uid = e_contact_get_const (contact, E_CONTACT_UID);
  if (uid)
    contacts_history_update_uid (CONTACTS_HISTORY (data->history), uid);

}

static void
on_moko_journal_entry_activated (ContactsHistory *history, 
                                 MokoJournalEntry *entry)
{
  g_print ("Launch Viewer\n");
}


