/* 
 *  Contacts - A small libebook-based address book.
 *
 *  Authored By Chris Lord <chris@o-hand.com>
 *
 *  Copyright (c) 2005 OpenedHand Ltd - http://o-hand.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libebook/e-book.h>

#include "contacts-defs.h"
#include "contacts-utils.h"
#include "contacts-callbacks-ui.h"
#include "contacts-edit-pane.h"
#include "contacts-main.h"

void
contacts_selection_cb (GtkTreeSelection * selection, ContactsData *data)
{
	GtkWidget *widget;
	EContact *contact;

	/* Get the currently selected contact and update the contact summary */
	contact = contacts_contact_from_selection (selection,
						   data->contacts_table);
	if (contact) {
		contacts_display_summary (contact, data->xml);
	} else {
		contacts_set_available_options (data->xml, TRUE, FALSE, FALSE);
		widget = glade_xml_get_widget (data->xml, "summary_vbox");
		gtk_widget_hide (widget);
	}
}

void
contacts_new_cb (GtkWidget *source, ContactsData *data)
{
	data->contact = e_contact_new ();	
	contacts_edit_pane_show (data, TRUE);
}

void
contacts_edit_cb (GtkWidget *source, ContactsData *data)
{
	/* Disable the new/edit/delete options and get the contact to edit */
	contacts_set_available_options (data->xml, FALSE, FALSE, FALSE);
	data->contact = contacts_get_selected_contact (data->xml,
						       data->contacts_table);
	data->changed = FALSE;
	
	contacts_edit_pane_show (data, FALSE);
}

void
contacts_treeview_edit_cb (GtkTreeView *treeview, GtkTreePath *arg1,
	GtkTreeViewColumn *arg2, ContactsData *data)
{
	/* Disable the new/edit/delete options and get the contact to edit */
	contacts_set_available_options (data->xml, FALSE, FALSE, FALSE);
	data->contact = contacts_get_selected_contact (data->xml,
						       data->contacts_table);
	data->changed = FALSE;
	
	contacts_edit_pane_show (data, FALSE);
}

void
contacts_delete_cb (GtkWidget *source, ContactsData *data)
{
	GtkWidget *dialog, *main_window;
	EContact *contact;
	gint result;
	GList *widgets;
	const gchar *name;
	
	contact = contacts_get_selected_contact (data->xml,
					data->contacts_table);
	if (!contact) return;

	name = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
	if (g_utf8_strlen (name, -1) <= 0)
		name = "Unknown";

	main_window = glade_xml_get_widget (data->xml, "main_window");
	dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
					 0, GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_CANCEL,
					 "Are you sure you want to delete "\
					 "'%s'?", name);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), "_Delete contact",
		GTK_RESPONSE_YES, NULL);
	
	widgets = contacts_set_widgets_desensitive (main_window);
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result) {
		case GTK_RESPONSE_YES:
			e_book_remove_contact (data->book, e_contact_get_const
				(contact, E_CONTACT_UID), NULL);
			break;
		default:
			break;
	}
	gtk_widget_destroy (dialog);
	contacts_set_widgets_sensitive (widgets);
}

void
contacts_import_cb (GtkWidget *source, ContactsData *data)
{
	GList *widgets;
	GtkFileFilter *filter;
	GtkWidget *main_window =
		glade_xml_get_widget (data->xml, "main_window");
	GtkWidget *dialog = gtk_file_chooser_dialog_new (
		"Import Contact",
		GTK_WINDOW (main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN,
		GTK_RESPONSE_ACCEPT,
		NULL);

	filter = gtk_file_filter_new ();
	gtk_file_filter_add_mime_type (filter, "text/plain");
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);
	
	widgets = contacts_set_widgets_desensitive (main_window);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename = gtk_file_chooser_get_filename 
					(GTK_FILE_CHOOSER (dialog));
		if (filename) {
			gchar *vcard_string;
			if (g_file_get_contents (
				filename, &vcard_string, NULL, NULL)) {
				EContact *contact =
					e_contact_new_from_vcard (vcard_string);
				if (contact) {
					e_book_add_contact (
						data->book, contact, NULL);
					g_object_unref (contact);
				}
				g_free (vcard_string);
			}
		}
	}
	
	contacts_set_widgets_sensitive (widgets);
	gtk_widget_destroy (dialog);
}

void
contacts_copy_cb (GtkWindow *main_window)
{
	GtkWidget *widget = gtk_window_get_focus (main_window);

	if (widget) {
		if (GTK_IS_EDITABLE (widget))
			gtk_editable_copy_clipboard (GTK_EDITABLE (widget));
		else if (GTK_IS_TEXT_VIEW (widget)) {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer (
				GTK_TEXT_VIEW (widget));
			gtk_text_buffer_copy_clipboard (buffer,
				gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
		} else if (GTK_IS_LABEL (widget)) {
			gint start, end;
			if (gtk_label_get_selection_bounds (GTK_LABEL (widget),
							    &start, &end)) {
				const gchar *text =
					gtk_label_get_text (GTK_LABEL (widget));
				gchar *start_text =
					g_utf8_offset_to_pointer (text, start);
				gchar *copy_text =
					g_strndup (start_text, end-start);
				gtk_clipboard_set_text (
				    gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
				    copy_text, end-start);
				g_free (copy_text);
			}
		}
	}
}

void
contacts_cut_cb (GtkWindow *main_window)
{
	GtkWidget *widget = gtk_window_get_focus (main_window);
	
	if (widget) {
		if (GTK_IS_EDITABLE (widget))
			gtk_editable_cut_clipboard (GTK_EDITABLE (widget));
		else if (GTK_IS_TEXT_VIEW (widget)) {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer (
				GTK_TEXT_VIEW (widget));
			gtk_text_buffer_cut_clipboard (buffer,
				gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
				TRUE);
		} else
			contacts_copy_cb (main_window);
	}
}

void
contacts_paste_cb (GtkWindow *main_window)
{
	GtkWidget *widget = gtk_window_get_focus (main_window);

	if (widget) {
		if (GTK_IS_EDITABLE (widget))
			gtk_editable_paste_clipboard (GTK_EDITABLE (widget));
		else if (GTK_IS_TEXT_VIEW (widget)) {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer (
				GTK_TEXT_VIEW (widget));
			gtk_text_buffer_paste_clipboard (buffer,
				gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
				NULL, TRUE);
		}
	}
}

void
contacts_about_cb (GtkWidget *dialog)
{
	gtk_widget_show (dialog);
}

gboolean
contacts_treeview_search_cb (GtkWidget *search_entry, GdkEventKey *event,
	GtkTreeView *treeview)
{
	gtk_widget_event (search_entry, (GdkEvent *)event);
	gtk_entry_set_position (GTK_ENTRY (search_entry), -1);
	
	return FALSE;
}

gboolean
contacts_is_row_visible_cb (GtkTreeModel * model, GtkTreeIter * iter,
			    GHashTable *contacts_table)
{
	gboolean result = FALSE;
	gchar *group;
	GList *groups, *g;
	const gchar *uid;
	EContactListHash *hash;
	GtkComboBox *groups_combobox;
	const gchar *search_string;
	GladeXML *xml;

	/* Check if the contact is in the currently selected group. */
	gtk_tree_model_get (model, iter, CONTACT_UID_COL, &uid, -1);
	if (!uid) return FALSE;
	hash = g_hash_table_lookup (contacts_table, uid);
	if (!hash || !hash->contact) return FALSE;
	xml = hash->xml;
	groups_combobox = GTK_COMBO_BOX (glade_xml_get_widget 
					 (xml, "groups_combobox"));
	group = gtk_combo_box_get_active_text (groups_combobox);
	groups = e_contact_get (hash->contact, E_CONTACT_CATEGORY_LIST);
	if (gtk_combo_box_get_active (groups_combobox) > 0) {
		for (g = groups; g; g = g->next) {
			if (strcmp (group, g->data) == 0)
				result = TRUE;
			g_free (g->data);
		}
		if (groups)
			g_list_free (groups);
	} else
		result = TRUE;
	g_free (group);
	if (!result)
		return FALSE;

	/* Search for any occurrence of the string in the search box in the 
	 * contact file-as name; if none is found, row isn't visible. Ignores 
	 * empty searches.
	 */
	search_string = gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget
							(xml, "search_entry")));
	if ((search_string) && (g_utf8_strlen (search_string, -1) > 0)) {
		gchar *name_string;
		gtk_tree_model_get (model, iter, CONTACT_NAME_COL, &name_string,
				    -1);
		if (name_string) {
			gunichar *isearch =
			    kozo_utf8_strcasestrip (search_string);
			if (!kozo_utf8_strstrcasestrip
			    (name_string, isearch))
				result = FALSE;
			g_free (name_string);
			g_free (isearch);
		}
	}
	return result;
}

gint
contacts_sort_treeview_cb (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b,
			   gpointer user_data)
{
	gchar *string1, *string2;
	gint returnval;

	gtk_tree_model_get (model, a, CONTACT_NAME_COL, &string1, -1);
	gtk_tree_model_get (model, b, CONTACT_NAME_COL, &string2, -1);
	if (!string1) string1 = g_new0 (gchar, 1);
	if (!string2) string2 = g_new0 (gchar, 1);
	returnval = strcasecmp ((const char *) string1,
				(const char *) string2);
	g_free (string1);
	g_free (string2);

	return returnval;
}

