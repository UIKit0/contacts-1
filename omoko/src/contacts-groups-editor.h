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

#ifndef GROUPS_EDITOR_H
#define GROUPS_EDITOR_H

void contacts_groups_pane_update_selection (GtkTreeSelection *selection, ContactsData *data);
void contacts_groups_pane_show (GtkWidget *button, ContactsData *data);
void contacts_groups_new_cb (GtkWidget *button, ContactsData *data);

void groups_checkbutton_cb (GtkWidget *checkbutton, ContactsData *data);
#endif
