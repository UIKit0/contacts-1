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

static GtkWidget *groups_create_dialog (ContactsData *data);

static GHashTable *groups_widgets_hash;
static GtkWidget *contact_label;
static GtkWidget *treeview;

void
moko_open_groups_editor (GtkWidget *widget, ContactsData *data)
{
	groups_widgets_hash = g_hash_table_new (NULL, NULL);
	if (!MOKO_IS_DIALOG_WINDOW (data->ui->chooser_dialog))
		data->ui->chooser_dialog = groups_create_dialog (data);

	moko_dialog_window_run (MOKO_DIALOG_WINDOW (data->ui->chooser_dialog));
	gtk_widget_hide (data->ui->chooser_dialog);
	g_hash_table_destroy (groups_widgets_hash);
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

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

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
}

static void
groups_contact_selection_cb  (GtkTreeSelection * selection, ContactsData *data)
{
	GtkWidget *widget;
	EContact *contact;
	GList *groups, *g;
	gchar *fullname, *markup;

	/* Get the currently selected contact and update the contact summary */
	contact = contacts_contact_from_selection (selection,
						   data->contacts_table);
	if (!contact)
		return;

	fullname = e_contact_get (contact, E_CONTACT_FULL_NAME);
	markup = g_markup_printf_escaped ("<span size=\"large\"><b>%s</b></span>", fullname);
	g_free (fullname);
	gtk_label_set_markup (GTK_LABEL (contact_label), markup);
	g_free (markup);

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

static GtkWidget *
groups_create_dialog (ContactsData *data)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *widget;
	GtkWidget *groups_vbox, *details_vbox;

	dialog = GTK_WIDGET (moko_dialog_window_new ());
	moko_dialog_window_set_title (MOKO_DIALOG_WINDOW (dialog), _("Group Membership") );

	vbox = gtk_vbox_new (FALSE, 0);
	moko_dialog_window_set_contents (MOKO_DIALOG_WINDOW (dialog), vbox);


	widget = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (widget), 24, 24, 24, 24);
	gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);

	details_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (widget), details_vbox);


	contact_label = gtk_label_new ("<span size=\"large\"><b>Contact Name</b></span>");
	gtk_box_pack_start (GTK_BOX (details_vbox), contact_label, FALSE, FALSE, 12);
	gtk_label_set_use_markup (GTK_LABEL (contact_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (contact_label), 0, 0);

	widget = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (details_vbox), widget, FALSE, FALSE, 0);

	groups_vbox = gtk_vbox_new (FALSE, 0);
	widget = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC,
						GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (widget), groups_vbox);
	gtk_box_pack_start (GTK_BOX (details_vbox), widget, TRUE, TRUE, 0);

	GList *cur;
	for (cur = data->contacts_groups; cur; cur = g_list_next (cur))
	{
		widget = gtk_check_button_new_with_label (cur->data);
		gtk_box_pack_start (GTK_BOX (groups_vbox), widget, TRUE, TRUE, 6);
		g_hash_table_insert (groups_widgets_hash, cur->data, widget);
		g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (groups_checkbutton_cb), data);
		gtk_widget_set_sensitive (GTK_WIDGET (widget), FALSE);
	}

	/* contacts list */
	GtkWidget *list = create_contacts_list (data);
	gtk_box_pack_start (GTK_BOX(vbox), list, FALSE, FALSE, 0);
	
	/* Connect signal for selection changed event */
	treeview = GTK_WIDGET (moko_navigation_list_get_tree_view (list));
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (groups_contact_selection_cb), data);


	gtk_widget_show_all (vbox);

	return dialog;

}


