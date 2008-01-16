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
#include <moko-finger-scroll.h>

void
create_contacts_history_page (ContactsData *data)
{

  GtkWidget *vbox, *scroll;

  vbox = gtk_vbox_new (FALSE, 0);

  data->history_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), data->history_label, FALSE, FALSE, 0);

  data->history = contacts_history_new ();
  scroll = moko_finger_scroll_new ();
  gtk_container_add (GTK_CONTAINER (scroll), data->history);

  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);

  contacts_notebook_add_page_with_icon (data->notebook, vbox, MOKO_STOCK_HISTORY);
}

void
contacts_history_page_free (ContactsData *data)
{
}

void
contacts_history_page_update (ContactsData *data)
{
  const gchar *s = NULL;
  gchar *markup;
  EContact *contact;

  contact = data->contact;

  if (!contact)
  {
    gtk_label_set_markup (GTK_LABEL (data->history_label), "<b>Communication History</b>");
    contacts_history_set_contact (CONTACTS_HISTORY (data->history), NULL);
    return;
  }

  /* Update the history widget */
  contacts_history_set_contact (CONTACTS_HISTORY (data->history), contact);

  /* set the title of the page */
  s = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
  if (s)
  {
    markup = g_markup_printf_escaped ("<b>%s</b>", s);
    gtk_label_set_markup (GTK_LABEL (data->history_label), markup);
    g_free (markup);
  }
  else
    gtk_label_set_markup (GTK_LABEL (data->history_label), "<b>Communication History</b>");

}

