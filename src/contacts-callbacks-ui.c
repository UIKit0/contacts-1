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
#include "config.h"
#ifdef HAVE_GNOMEVFS
#include <libgnomevfs/gnome-vfs.h>
#endif

#include "contacts-defs.h"
#include "contacts-utils.h"
#include "contacts-callbacks-ui.h"
#include "contacts-callbacks-ebook.h"
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
	contacts_edit_cb (GTK_WIDGET (treeview), data);
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
			/* TODO: add callback to handle errors */
			e_book_async_remove_contact (data->book, contact, NULL, NULL);
			break;
		default:
			break;
	}
	gtk_widget_destroy (dialog);
	contacts_set_widgets_sensitive (widgets);
}

void
contacts_import (ContactsData *data, const gchar *filename, gboolean do_confirm)
{
	gchar *vcard_string;
#ifdef HAVE_GNOMEVFS
	int size;
	GnomeVFSResult vfs_result;
	
	vfs_result = gnome_vfs_read_entire_file (
		filename, &size, &vcard_string);
	if (vfs_result == GNOME_VFS_OK) {
#else
	if (g_file_get_contents (
		filename, &vcard_string, NULL, NULL)) {
#endif
		EContact *contact =
			e_contact_new_from_vcard (vcard_string);
		if (contact) {
			gint result = GTK_RESPONSE_YES;
			if (do_confirm) {
				GtkWidget *dialog, *main_window;
				GList *widgets;
				
				main_window = glade_xml_get_widget (
					data->xml, "main_window");
				dialog = gtk_message_dialog_new (
					GTK_WINDOW (main_window),
					0, GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_NONE,
					"Would you like to import contact "\
					"'%s'?",
					(const char *)e_contact_get_const (
						contact, E_CONTACT_FULL_NAME));
				gtk_dialog_add_buttons (GTK_DIALOG (dialog),
					"_Show contact", GTK_RESPONSE_NO,
					"_Import contact", GTK_RESPONSE_YES,
					NULL);
				widgets = contacts_set_widgets_desensitive (
					main_window);
				result = gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				contacts_set_widgets_sensitive (widgets);
				g_list_free (widgets);
			}
			if (result == GTK_RESPONSE_YES) {
				GList *lcontact =
					g_list_prepend (NULL, contact);
				/* Add contact to db and select it */
				e_book_add_contact (
					data->book, contact, NULL);
				/* Maually trigger the added callback so that
				 * the contact can be selected.
				 */
				contacts_added_cb (data->book_view, lcontact,
					data);
				contacts_set_selected_contact (data->xml,
					(const gchar *)e_contact_get_const (
						contact, E_CONTACT_UID));
				g_list_free (lcontact);
			} else {
				contacts_display_summary (contact, data->xml);
				contacts_set_available_options (
					data->xml, TRUE, FALSE, FALSE);
			}
			g_object_unref (contact);
		}
		g_free (vcard_string);
	}
#ifdef HAVE_GNOMEVFS
	else {
		g_warning ("Error loading '%s': %s",
			filename, gnome_vfs_result_to_string (vfs_result));
	}
#endif
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
	gtk_file_filter_add_mime_type (filter, "text/x-vcard");
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);
#ifdef HAVE_GNOMEVFS
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);
#endif
	
	widgets = contacts_set_widgets_desensitive (main_window);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename = gtk_file_chooser_get_filename 
					(GTK_FILE_CHOOSER (dialog));
		if (filename) {
			contacts_import (data, filename, FALSE);
			g_free (filename);
		}
	}
	
	contacts_set_widgets_sensitive (widgets);
	gtk_widget_destroy (dialog);
	g_list_free (widgets);
}

void
contacts_export (ContactsData *data, const gchar *filename)
{
	char *vcard = e_vcard_to_string (
		E_VCARD (data->contact), EVC_FORMAT_VCARD_30);
		
	if (vcard) {
#ifdef HAVE_GNOMEVFS
		GnomeVFSHandle *file;
		GnomeVFSFileSize bytes_written;
		if (gnome_vfs_open (&file, filename, GNOME_VFS_OPEN_WRITE) ==
		    GNOME_VFS_OK) {
			if (gnome_vfs_write (file, vcard, strlen (vcard),
			    &bytes_written) != GNOME_VFS_OK)
				g_warning ("Writing to '%s' failed, %lld bytes "
					"written", filename,
					bytes_written);
			gnome_vfs_close (file);
		}
#else
		FILE *file = fopen (filename, "w");
		if (file) {
			fputs (vcard, file);
			fclose (file);
		}
#endif
		g_free (vcard);
	}
}

void
contacts_export_cb (GtkWidget *source, ContactsData *data)
{
	GList *widgets;
	GtkWidget *main_window =
		glade_xml_get_widget (data->xml, "main_window");
	GtkWidget *dialog = gtk_file_chooser_dialog_new (
		"Export Contact",
		GTK_WINDOW (main_window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE,
		GTK_RESPONSE_ACCEPT,
		NULL);
	
	/* Gtk 2.8 feature */
/*	gtk_file_chooser_set_do_overwrite_confirmation (
		GTK_FILE_CHOOSER (dialog), TRUE);*/
#ifdef HAVE_GNOMEVFS
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);
#endif

	widgets = contacts_set_widgets_desensitive (main_window);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename = gtk_file_chooser_get_filename 
					(GTK_FILE_CHOOSER (dialog));
		if (filename) {
#ifdef HAVE_GNOMEVFS
			GnomeVFSURI *uri = gnome_vfs_uri_new (filename);
			if (gnome_vfs_uri_exists (uri))
#else
			if (g_file_test (
			    filename, G_FILE_TEST_EXISTS))
#endif
			{
				GtkWidget *button, *image;
				GtkWidget *overwrite_dialog =
				gtk_message_dialog_new_with_markup (
					GTK_WINDOW (dialog),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_CANCEL,
					"<big><b>The file \"%s\""
					" already exists.\n"
					"Do you want to replace it?</b></big>",
					filename);
				gtk_message_dialog_format_secondary_markup (
					GTK_MESSAGE_DIALOG (overwrite_dialog),
					"Replacing it will overwrite its "
					"contents.");
				button = gtk_dialog_add_button (
					GTK_DIALOG (overwrite_dialog),
					"_Replace",
					GTK_RESPONSE_OK);
				image = gtk_image_new_from_stock (
					GTK_STOCK_SAVE_AS,
					GTK_ICON_SIZE_BUTTON);
				gtk_button_set_image (
					GTK_BUTTON (button), image);
				
				if (gtk_dialog_run (
				     GTK_DIALOG (overwrite_dialog)) ==
				      GTK_RESPONSE_OK)
					contacts_export (data, filename);
				
				gtk_widget_destroy (overwrite_dialog);
			} else
				contacts_export (data, filename);
			
			g_free (filename);
#ifdef HAVE_GNOMEVFS
			gnome_vfs_uri_unref (uri);
#endif
		}
	}
	
	contacts_set_widgets_sensitive (widgets);
	gtk_widget_destroy (dialog);
	g_list_free (widgets);
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
	if (GTK_WIDGET_VISIBLE (glade_xml_get_widget (
	    xml, "search_entry_hbox"))) {
		search_string = gtk_entry_get_text (
			GTK_ENTRY (glade_xml_get_widget (xml, "search_entry")));
		if ((search_string) &&
		    (g_utf8_strlen (search_string, -1) > 0)) {
			gchar *name_string;
			gtk_tree_model_get (model, iter, CONTACT_NAME_COL,
				&name_string, -1);
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
	} else {
		gint i;
		gchar *name, *uname;
		gunichar c;
		GSList *b, *buttons = gtk_radio_button_get_group (
			GTK_RADIO_BUTTON (glade_xml_get_widget (
				xml, "symbols_radiobutton")));
		
		/* Find the active radio button */
		for (b = buttons, i = 0; b; b = b->next, i++)
			if (gtk_toggle_button_get_active (
				GTK_TOGGLE_BUTTON (b->data))) break;
		
		gtk_tree_model_get (model, iter, CONTACT_NAME_COL,
			&name, -1);
		uname = g_utf8_strup (name, -1);
		c = g_utf8_get_char (uname);
		
		switch (i) {
			case 4 :
				if (c >= 'A' || c <= 'Z')
					return FALSE;
				break;
			case 3 :
				if (c < 'A' || c > 'G')
					return FALSE;
				break;
			case 2 :
				if (c < 'H' || c > 'N')
					return FALSE;
				break;
			case 1 :
				if (c < 'O' || c > 'U')
					return FALSE;
				break;
			case 0 :
				if (c < 'V' || c > 'Z')
					return FALSE;
				break;
			default :
				g_warning ("Unknown search tab state %d", i);
				break;
		}
		
		g_free (name);
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

