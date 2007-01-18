/*
 * contacts-omoko-groups-editor.c
 * This file is part of Contacts
 *
 * Copyright (C) 2006 - OpenedHand Ltd
 *
 * Contacts is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Contacts is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Contacts; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include "contacts-defs.h"
#include "contacts-omoko.h"
#include "contacts-main.h"
#include "contacts-utils.h"

static void groups_checkbutton_cb (GtkWidget *checkbutton, ContactsData *data);
void contacts_groups_pane_update_selection (GtkTreeSelection *selection, ContactsData *data);

/* TODO: put these in a struct and stop using global variables */
static GHashTable *groups_widgets_hash;

void
contacts_groups_pane_show (GtkWidget *button, ContactsData *data)
{
	GtkWidget *widget;
	GList *cur;
	if (!groups_widgets_hash)
	{
		groups_widgets_hash = g_hash_table_new (NULL, NULL);
		for (cur = data->contacts_groups; cur; cur = g_list_next (cur))
		{
			widget = gtk_check_button_new_with_label (cur->data);
			gtk_box_pack_start (GTK_BOX (data->ui->groups_vbox), widget, FALSE, FALSE, 6);
			g_hash_table_insert (groups_widgets_hash, cur->data, widget);
			g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (groups_checkbutton_cb), data);
			gtk_widget_show (widget);
		}
	}
	gtk_notebook_set_current_page (GTK_NOTEBOOK (data->ui->main_notebook), CONTACTS_GROUPS_PANE);

	contacts_groups_pane_update_selection (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->ui->contacts_treeview)), data);
}

void
contacts_groups_pane_update_selection (GtkTreeSelection *selection, ContactsData *data)
{
	GtkWidget *widget;
	EContact *contact;
	GList *groups, *g;


	if (!selection)
		return;

	/* Get the currently selected contact and update the contact summary */
	contact = contacts_contact_from_selection (selection,
						   data->contacts_table);
	if (!contact)
		return;

	groups = e_contact_get (contact, E_CONTACT_CATEGORY_LIST);
	for (g = data->contacts_groups; g; g = g_list_next (g))
	{
		widget = g_hash_table_lookup (groups_widgets_hash, g->data);
		gtk_widget_set_sensitive (GTK_WIDGET (widget), TRUE);

		if (g_list_find_custom (groups, g->data, (GCompareFunc) strcmp))
		{
			/* make sure we don't set updating flag unless it is actually going to change */
			if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
			{
				g_object_set_data (G_OBJECT (widget), "updating", GINT_TO_POINTER (TRUE));
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
			}
		}
		else
		{
			/* make sure we don't set updating flag unless it is actually going to change */
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
			{
				g_object_set_data (G_OBJECT (widget), "updating", GINT_TO_POINTER (TRUE));
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
			}
		}
	}


}

void
contacts_groups_pane_hide ()
{
	g_hash_table_destroy (groups_widgets_hash);
}


void
contacts_groups_new_group_cb (GtkWidget *button, ContactsData *data)
{
	GtkWidget *widget;
	gchar *text;
	GtkTreeSelection *selection;
	GtkWidget *input_dialog;

	input_dialog = gtk_dialog_new_with_buttons ("New Group", NULL, GTK_DIALOG_MODAL,
							GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_has_separator (GTK_DIALOG (input_dialog), FALSE);

	widget = gtk_entry_new ();
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (input_dialog)->vbox), widget);
	gtk_widget_show (widget);

	gtk_dialog_run (GTK_DIALOG (input_dialog));

	text = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
	gtk_widget_hide (input_dialog);

	/* add the group to the list of groups */
	data->contacts_groups = g_list_append (data->contacts_groups, text);

	/* FIXME: need to make sure update groups is called here */

	widget = gtk_check_button_new_with_label (text);
	gtk_box_pack_start (GTK_BOX (data->ui->groups_vbox), widget, FALSE, FALSE, 0);
	g_hash_table_insert (groups_widgets_hash, text, widget);
	g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (groups_checkbutton_cb), data);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->ui->contacts_treeview));
	if (gtk_tree_selection_count_selected_rows (selection) == 0)
	{
		/* no contact selected, so just disable for now */
		gtk_widget_set_sensitive (widget, FALSE);
	}
	else
	{
		/* add the new group to the current contact */
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
	}


	gtk_widget_show (widget);
}

static void
groups_checkbutton_cb (GtkWidget *checkbutton, ContactsData *data)
{
	EContact *contact;
	GtkTreeSelection *selection;
	GList *current_groups, *g = NULL;
	gchar *new_group;

	if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (checkbutton), "updating")))
	{
		g_object_set_data (G_OBJECT(checkbutton), "updating", GINT_TO_POINTER (FALSE));
		return;
	}

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->ui->contacts_treeview));

	contact = contacts_contact_from_selection (selection,
						   data->contacts_table);
	current_groups = e_contact_get (contact, E_CONTACT_CATEGORY_LIST);

	/* TODO: probably ought to do something better here */
	new_group = g_strdup(gtk_button_get_label (GTK_BUTTON (checkbutton)));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)))
	{
		/* add this to the contact */
		current_groups = g_list_append (current_groups, new_group);
	}
	else
	{
		/* make sure this isn't in the list */
		g = g_list_find_custom (current_groups, new_group, (GCompareFunc) strcmp);
		if (g)
			current_groups = g_list_remove (current_groups, g->data);
	}

	e_contact_set (contact, E_CONTACT_CATEGORY_LIST, current_groups);
	e_book_async_commit_contact (data->book, contact, NULL, NULL);
}
