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
#include <libebook/e-book.h>
#include <hito-contact-store.h>

#define PADDING 6

enum {
  LIST_PAGE_NUM,
  DETAIL_PAGE_NUM,
  HISTORY_PAGE_NUM,
  GROUPS_PAGE_NUM
};

typedef struct
{
  GtkWidget *notebook;
  GtkWidget *window;

  EBook *book;
  EBookView *view;

  GtkTreeModel *contact_store;

  /* current contact being edited - must be set through contact_set_current_contact() */
  EContact *contact;

  GtkTreeModel *contacts_store;

  /* this should be set to true by any function that modifies the above contact */
  gboolean dirty;

  /* the following should be considered private */

  /* index page */
  GtkWidget *search_entry;
  GtkWidget *groups_combo;
  GtkWidget *contacts_treeview;
  GtkToolItem *dial_button;
  GtkToolItem *sms_button;

  /* details page */
  GtkWidget *photo;
  GtkWidget *fullname;
  GtkWidget *org;
  GtkWidget *telephone;
  GtkWidget *email;
  GtkWidget *add_email_button;
  GtkWidget *add_telephone_button;
  GtkWidget *address;
  GtkToolItem *edit_toggle;

  GtkListStore *attribute_liststore;
  gboolean detail_page_loading;

  /* history page */
  GtkWidget *history;
  GtkWidget *history_label;

  /* groups page */
  GtkWidget *groups;
  GtkTreeModel *groups_liststore;
  GtkWidget *groups_label;
  GtkWidget *groups_box;

} ContactsData;

void contacts_notebook_add_page_with_icon (GtkWidget *notebook, GtkWidget *child, const gchar *icon_name);
void contacts_set_current_contact (ContactsData *data, EContact *contact);

