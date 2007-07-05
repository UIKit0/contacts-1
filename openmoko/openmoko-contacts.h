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


typedef struct
{
  GtkWidget *notebook;
  GtkWidget *window;

  EBook *book;
  EBookView *view;

  /* the following should be considered private */

  /* index page */
  GtkWidget *search_entry;
  GtkWidget *groups_combo;

  /* details page */
  GtkWidget *photo;
  GtkWidget *fullname;
  GtkWidget *org;
  GtkWidget *telephone;
  GtkWidget *email;
  GtkWidget *add_email_button;
  GtkWidget *add_telephone_button;

} ContactsData;


