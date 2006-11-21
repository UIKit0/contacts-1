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
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>
#include "config.h"
#ifdef HAVE_GNOMEVFS
#include <libgnomevfs/gnome-vfs.h>
#endif
#ifdef HAVE_GCONF
#include <gconf/gconf-client.h>
#endif

#include "bacon-message-connection.h"
#include "contacts-defs.h"
#include "contacts-utils.h"
#include "contacts-main.h"
#include "contacts-callbacks-ui.h"
#include "contacts-callbacks-ebook.h"
#include "contacts-edit-pane.h"
#include "contacts-ui.h"

#define GCONF_PATH "/apps/contacts"
#define GCONF_KEY_SEARCH "/apps/contacts/search_type"

void
contacts_update_treeview (ContactsData *data, GtkWidget *source)
{
	GtkTreeView *view;
	GtkTreeModelFilter *model;
	gint visible_rows;

	view = GTK_TREE_VIEW (data->ui->contacts_treeview);
	model = GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (view));
	gtk_tree_model_filter_refilter (model);
	
	visible_rows = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (model),
						       NULL);
	/* If there's only one visible contact, select it */
	if (visible_rows == 1) {
		GtkTreeSelection *selection =
					gtk_tree_view_get_selection (view);
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model),
						   &iter)) {
			gtk_tree_selection_select_iter (selection, &iter);
		}
	}
}

void
contacts_display_summary (EContact *contact, ContactsData *contacts_data)
{
	GtkWidget *widget;
	const gchar *string;
	GtkImage *photo;
	GList *a, *groups, *attributes;
	gchar *name_markup, *groups_text = NULL;
	GValue *can_focus = g_new0 (GValue, 1);

	if (!E_IS_CONTACT (contact))
		return;

	/* Retrieve contact name and groups */
	widget = contacts_data->ui->summary_name_label;
	string = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
	/* Only examine 4-bytes (maximum UTF-8 character width is 4 bytes?) */
	if ((!string) || (g_utf8_strlen (string, 4) <= 0))
		string = _("Unnamed");
	groups = e_contact_get (contact, E_CONTACT_CATEGORY_LIST);
	groups_text = contacts_string_list_as_string (groups, ", ", FALSE);
	name_markup = g_markup_printf_escaped (
		"<big><b>%s</b></big>\n<small>%s</small>",
		string ? string : "", groups_text ? groups_text : "");
	gtk_label_set_markup (GTK_LABEL (widget), name_markup);
	if (groups) {
		g_list_free (groups);
		g_free (groups_text);
	}
	g_free (name_markup);

	/* Retrieve contact picture and resize */
	widget = contacts_data->ui->photo_image;
	photo = contacts_load_photo (contact);
	if ((gtk_image_get_storage_type (photo) == GTK_IMAGE_EMPTY) ||
	    (gtk_image_get_storage_type (photo) == GTK_IMAGE_PIXBUF))
		gtk_image_set_from_pixbuf (GTK_IMAGE (widget),
					  gtk_image_get_pixbuf (photo));
	else if (gtk_image_get_storage_type 
		 (photo) == GTK_IMAGE_ICON_NAME) {
		const gchar *icon_name;
		GtkIconSize size;
		gtk_image_get_icon_name (photo, &icon_name, &size);
		gtk_image_set_from_icon_name (GTK_IMAGE (widget), icon_name,
					      size);
	}
	gtk_widget_destroy (GTK_WIDGET (photo));

	/* Create summary (displays fields marked as REQUIRED) */
	widget = contacts_data->ui->summary_table;
	gtk_container_foreach (GTK_CONTAINER (widget),
			       (GtkCallback)contacts_remove_edit_widgets_cb,
			       widget);
	g_object_set (widget, "n-rows", 1, NULL);
	g_object_set (widget, "n-columns", 2, NULL);
	attributes = e_vcard_get_attributes (E_VCARD (contact));
	g_value_init (can_focus, G_TYPE_BOOLEAN);
	g_value_set_boolean (can_focus, FALSE);
	for (a = attributes; a; a = a->next) {
		GtkWidget *name_widget, *value_widget;
		gchar *value_text, *name_markup;
		GList *values;
		const gchar **types;
		const gchar *attr_name;
		EVCardAttribute *attr = (EVCardAttribute *)a->data;
		const ContactsField *field = contacts_get_contacts_field (
			e_vcard_attribute_get_name (attr));
			
		if (!field || field->priority >= REQUIRED)
			continue;
		
		values = e_vcard_attribute_get_values (attr);
		value_text =
			contacts_string_list_as_string (values, "\n", FALSE);
		
		attr_name = e_vcard_attribute_get_name (attr);
		types = contacts_get_field_types (attr_name);
		if (types) {
			gchar *types_string = NULL;
			const gchar **valid_types;
			GList *types_list = NULL;
			guint i;

			types_list = contacts_get_type_strings (
				e_vcard_attribute_get_params (attr));
			if (types_list) {
				valid_types =
					contacts_get_field_types (attr_name);
				if (g_ascii_strncasecmp (types_list->data,
				    "X-", 2) == 0)
					types_string = (gchar *)
							(types_list->data)+2;
				else if (valid_types) {
					for (i = 0; valid_types[i]; i++) {
						if (g_ascii_strcasecmp (
							types_list->data,
							valid_types[i]) == 0) {
							types_string = (gchar *)
								valid_types[i];
							break;
						}
					}
				}
			}
			if (!types_string)
				types_string = _("Other");
			
			name_markup = g_strdup_printf (
				"<b>%s:</b>\n<small>(%s)</small>",
				contacts_field_pretty_name (field),
				types_string);
				
			g_list_free (types_list);
		} else {
			name_markup = g_strdup_printf ("<b>%s:</b>",
				contacts_field_pretty_name (field));
		}
		name_widget = gtk_label_new (name_markup);
		gtk_label_set_use_markup (GTK_LABEL (name_widget), TRUE);
		value_widget = gtk_label_new (value_text);
		gtk_label_set_selectable (GTK_LABEL (value_widget), TRUE);
		gtk_label_set_justify (GTK_LABEL (name_widget),
				       GTK_JUSTIFY_RIGHT);
		gtk_misc_set_alignment (GTK_MISC (name_widget), 1, 0);
		gtk_misc_set_alignment (GTK_MISC (value_widget), 0, 0);
/*		g_object_set_property (G_OBJECT (name_widget),
			"can-focus", can_focus);
		g_object_set_property (G_OBJECT (value_widget),
			"can-focus", can_focus);*/
		
		contacts_append_to_edit_table (GTK_TABLE (widget), name_widget,
					       value_widget, FALSE);
		
		g_free (name_markup);
		g_free (value_text);
	}
	g_value_unset (can_focus);
	g_free (can_focus);

	widget = contacts_data->ui->summary_vbox;
	gtk_widget_show (widget);
	contacts_set_available_options (contacts_data, TRUE, TRUE, TRUE);

	widget = contacts_data->ui->summary_table;
	contacts_remove_labels_from_focus_chain (GTK_CONTAINER (widget));
}
static void
start_query (EBook *book, EBookStatus status, EBookView *book_view,
	gpointer closure)
{
	ContactsData *contacts_data = closure;
	if (status == E_BOOK_ERROR_OK) {
		g_object_ref (book_view);
		contacts_data->book_view = book_view;

		/* Connect signals on EBookView */
		g_signal_connect (G_OBJECT (book_view), "contacts_added",
			G_CALLBACK (contacts_added_cb), contacts_data);
		g_signal_connect (G_OBJECT (book_view), "contacts_changed",
			G_CALLBACK (contacts_changed_cb), contacts_data);
		g_signal_connect (G_OBJECT (book_view), "contacts_removed",
			G_CALLBACK (contacts_removed_cb), contacts_data);
		
		e_book_view_start (book_view);
	} else {
		g_warning("Got error %d when getting book view", status);
	}
}

static void
opened_book (EBook *book, EBookStatus status, gpointer closure)
{
	ContactsData *contacts_data = closure;
	EBookQuery *query;
	
	if (status == E_BOOK_ERROR_OK) {
		query = e_book_query_any_field_contains ("");
		e_book_async_get_book_view (contacts_data->book, query, NULL,
			-1, start_query, contacts_data);
		e_book_query_unref (query);
	} else {
		g_warning("Got error %d when opening book", status);
	}
}

static gboolean
open_book (gpointer data)
{
	ContactsData *contacts_data = data;
	e_book_async_open (
		contacts_data->book, FALSE, opened_book, contacts_data);
	return FALSE;
}

static gboolean
contacts_import_from_param (gpointer data)
{
	ContactsData *contacts_data = data;
	
	g_printf ("Opening '%s'\n", contacts_data->file);
	contacts_import (contacts_data, contacts_data->file, TRUE);
	
	return FALSE;
}

static void
contacts_bacon_cb (const char *message, gpointer user_data)
{
	ContactsData *data = user_data;
	
	if (!message)
		return;
	
	gtk_window_present (GTK_WINDOW (data->ui->main_window));
	if (message[0] != ':') {
		g_printf ("Opening '%s'\n", message);
		contacts_import (data, message, TRUE);
	}
}

#ifdef HAVE_GCONF
static void
contacts_gconf_search_cb (GConfClient *client, guint cnxn_id, GConfEntry *entry,
	gpointer user_data)
{
	const gchar *search;
	GConfValue *value;
	GtkWidget *widget;
	ContactsData *contacts_data = user_data;
		
	value = gconf_entry_get_value (entry);
	search = gconf_value_get_string (value);
	if (strcmp (search, "entry") == 0) {
		widget = contacts_data->ui->search_entry_hbox;
		gtk_widget_show (widget);
		widget = contacts_data->ui->search_tab_hbox;
		gtk_widget_hide (widget);
	} else if (strcmp (search, "alphatab") == 0) {
		widget = contacts_data->ui->search_entry_hbox;
		gtk_widget_hide (widget);
		widget = contacts_data->ui->search_tab_hbox;
		gtk_widget_show (widget);
	} else {
		g_warning ("Unknown search UI type \"%s\"", search);
	}
}
#endif

int
main (int argc, char **argv)
{
	BaconMessageConnection *mc;
#ifdef HAVE_GCONF
	const char *search;
	GConfClient *client;
#endif
	ContactsData *contacts_data;	/* Variable for passing around data -
					 * see contacts-defs.h.
					 */
	GOptionContext *context;
	static gint plug = 0;
	GtkWidget *widget;
	
	static GOptionEntry entries[] = {
		{ "plug", 'p', 0, G_OPTION_ARG_INT, &plug,
			"Socket ID of an XEmbed socket to plug into", NULL },
		{ NULL }
	};

        /* Initialise the i18n support code */
        bindtextdomain (GETTEXT_PACKAGE, CONTACTS_LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

	context = g_option_context_new (" - A light-weight address-book");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_parse (context, &argc, &argv, NULL);

	mc = bacon_message_connection_new ("contacts");
	if (!bacon_message_connection_get_is_server (mc)) {
		bacon_message_connection_send (mc, argv[1] ? argv[1] : ":");
		bacon_message_connection_free (mc);
		gdk_notify_startup_complete ();
		return 0;
	}

	contacts_data = g_new0 (ContactsData, 1);
	contacts_data->ui = g_new0 (ContactsUI, 1);
	bacon_message_connection_set_callback (
		mc, contacts_bacon_cb, contacts_data);

	/* Set critical errors to close application */
	g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);

	/* Load the system addressbook */
	contacts_data->book = e_book_new_system_addressbook (NULL);
	if (!contacts_data->book)
		g_critical ("Could not load system addressbook");


#ifdef HAVE_GNOMEVFS
	gnome_vfs_init ();
#endif
#ifdef HAVE_GCONF
	client = gconf_client_get_default ();
	search = gconf_client_get_string (client, GCONF_KEY_SEARCH, NULL);
	if (!search) {
		gconf_client_set_string (
			client, GCONF_KEY_SEARCH, "entry", NULL);
	} else if (strcmp (search, "alphatab") == 0) {
		widget = contacts_data->ui->search_entry_hbox;
		gtk_widget_hide (widget);
		widget = contacts_data->ui->search_tab_hbox;
		gtk_widget_show (widget);
	}
	gconf_client_add_dir (client, GCONF_PATH, GCONF_CLIENT_PRELOAD_NONE,
		NULL);
	gconf_client_notify_add (client, GCONF_KEY_SEARCH,
		contacts_gconf_search_cb, xml, NULL, NULL);
#endif

	contacts_data->contacts_table = g_hash_table_new_full (g_str_hash,
						g_str_equal, NULL, 
						(GDestroyNotify)
						 contacts_free_list_hash);

	/* Setup the ui */
	contacts_setup_ui (contacts_data);

	/* Start */
	g_idle_add (open_book, contacts_data);
	if (argv[1] != NULL) {
		contacts_data->file = argv[1];
		g_idle_add (contacts_import_from_param, contacts_data);
	}
	
	widget = contacts_data->ui->main_window;
	if (plug > 0) {
		GtkWidget *plug_widget;
		GtkWidget *contents;
		
		g_debug ("Plugging into socket %d", plug);
		plug_widget = gtk_plug_new (plug);
		contents = g_object_ref (gtk_bin_get_child (GTK_BIN (widget)));
		gtk_container_remove (GTK_CONTAINER (widget), contents);
		gtk_container_add (GTK_CONTAINER (plug_widget), contents);
		g_object_unref (contents);
		g_signal_connect (G_OBJECT (plug_widget), "destroy",
				  G_CALLBACK (gtk_main_quit), NULL);
		widget = contacts_data->ui->main_menubar;
		gtk_widget_hide (widget);
		gtk_widget_show (plug_widget);
	} else {
		g_signal_connect (G_OBJECT (widget), "destroy",
				  G_CALLBACK (gtk_main_quit), NULL);
		gtk_widget_show (widget);
	}

	/* fix icon sizes to 16x16 for the moment... */
	gtk_rc_parse_string ("gtk_icon_sizes=\"gtk-button=16,16:gtk-menu=16,16\"");

	gtk_main ();

	/* Unload the addressbook */
	e_book_view_stop (contacts_data->book_view);
	g_object_unref (contacts_data->book_view);
	g_object_unref (contacts_data->book);
	g_free (contacts_data->ui);

	return 0;
}
