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

#include <gtk/gtk.h>
#include <moko-stock.h>

#include "openmoko-contacts.h"

void
create_contacts_groups_page (ContactsData *data)
{
  GtkWidget *box;

  box = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (data->notebook), box, gtk_image_new_from_stock (MOKO_STOCK_CONTACT_GROUPS, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_container_child_set (GTK_CONTAINER (data->notebook), box, "tab-expand", TRUE, "tab-fill", TRUE, NULL);

  data->groups_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (box), data->groups_label, FALSE, FALSE, 0);
}

void
free_contacts_groups_page (ContactsData *data)
{
}

void
contacts_groups_page_set_contact (ContactsData *data, EContact *contact)
{
  const gchar *s;

  /* set the title of the page */
  s = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
  if (s)
  {
    gchar *markup;
    markup = g_markup_printf_escaped ("<b>%s</b>", s);
    gtk_label_set_markup (GTK_LABEL (data->groups_label), markup);
    g_free (markup);
  }
  else
    gtk_label_set_markup (GTK_LABEL (data->groups_label), "<b>Communication History</b>");


}


