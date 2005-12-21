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
#include <glade/glade.h>
#include <libebook/e-book.h>

#include "contacts-defs.h"
#include "contacts-utils.h"
#include "contacts-callbacks-ui.h"
#include "contacts-callbacks-ebook.h"
#include "contacts-edit-pane.h"

#define XML_FILE PKGDATADIR "/contacts.glade"

void
contacts_update_treeview (GtkWidget *source)
{
	GtkTreeView *view;
	GtkTreeModelFilter *model;
	gint visible_rows;
	GladeXML *xml = glade_get_widget_tree (source);

	view = GTK_TREE_VIEW (glade_xml_get_widget (xml, "contacts_treeview"));
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
contacts_search_changed_cb (GtkWidget *search_entry)
{
	gtk_widget_grab_focus (search_entry);
	contacts_update_treeview (search_entry);
}

static void
contacts_remove_labels_from_focus_chain (GtkContainer *container)
{
	GList *chain, *l;
	
	gtk_container_get_focus_chain (container, &chain);
	
	for (l = chain; l; l = l->next) {
		if (GTK_IS_LABEL (l->data)) {
			gconstpointer data = l->data;
			l = l->prev;
			chain = g_list_remove (chain, data);
		}
	}
	
	gtk_container_set_focus_chain (container, chain);
	g_list_free (chain);
}

void
contacts_display_summary (EContact *contact, GladeXML *xml)
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
	widget = glade_xml_get_widget (xml, "summary_name_label");
	string = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
	if ((!string) || (g_utf8_strlen (string, -1) <= 0))
		string = "Unnamed";
	groups = e_contact_get (contact, E_CONTACT_CATEGORY_LIST);
	groups_text = contacts_string_list_as_string (groups, ", ", FALSE);
	name_markup = g_strdup_printf
		("<big><b>%s</b></big>\n<small>%s</small>",
		 string ? string : "", groups_text ? groups_text : "");
	gtk_label_set_markup (GTK_LABEL (widget), name_markup);
	if (groups) {
		g_list_free (groups);
		g_free (groups_text);
	}
	g_free (name_markup);

	/* Retrieve contact picture and resize */
	widget = glade_xml_get_widget (xml, "photo_image");
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
	widget = glade_xml_get_widget (xml, "summary_table");
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
				types_string = "Other";
			
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
					       value_widget);
		
		g_free (name_markup);
		g_free (value_text);
	}
	g_value_unset (can_focus);
	g_free (can_focus);

	widget = glade_xml_get_widget (xml, "summary_vbox");
	gtk_widget_show (widget);
	contacts_set_available_options (xml, TRUE, TRUE, TRUE);

	widget = glade_xml_get_widget (xml, "summary_table");
	contacts_remove_labels_from_focus_chain (GTK_CONTAINER (widget));
}

static void
chooser_toggle_cb (GtkCellRendererToggle * cell,
		   gchar * path_string, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL (user_data);

	gtk_tree_model_get_iter_from_string (model, &iter, path_string);
	if (gtk_cell_renderer_toggle_get_active (cell))
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    CHOOSER_TICK_COL, FALSE, -1);
	else
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    CHOOSER_TICK_COL, TRUE, -1);
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

int
main (int argc, char **argv)
{
/*	GValue *can_focus = g_new0 (GValue, 1);*/
	GtkWidget *widget;		/* Variables for UI initialisation */
	GtkComboBox *groups_combobox;
	GtkTreeView *contacts_treeview;
	GtkTreeSelection *selection;
	GtkListStore *model;
	GtkTreeModelFilter *filter;
	GtkCellRenderer *renderer;
	GladeXML *xml;			/* */
	ContactsData *contacts_data;	/* Variable for passing around data -
					 * see contacts-defs.h.
					 */

	contacts_data = g_new0 (ContactsData, 1);

	/* Standard initialisation for gtk and glade */
	gtk_init (&argc, &argv);
	glade_init ();

	/* Set critical errors to close application */
	g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);

	/* Load up main_window from interface xml, quit if xml file not found */
	xml = glade_xml_new (XML_FILE, NULL, NULL);
	if (!xml)
		g_critical ("Could not find interface XML file '%s'",
			    XML_FILE);

	/* Load the system addressbook */
	contacts_data->book = e_book_new_system_addressbook (NULL);
	if (!contacts_data->book)
		g_critical ("Could not load system addressbook");

	/* Hook up signals defined in interface xml */
	glade_xml_signal_autoconnect (xml);
	
	contacts_data->contacts_table = g_hash_table_new_full (g_str_hash,
						g_str_equal, NULL, 
						(GDestroyNotify)
						 contacts_free_list_hash);
	contacts_data->xml = xml;

	/* Add the column to the GtkTreeView */
	contacts_treeview =
	    GTK_TREE_VIEW (glade_xml_get_widget
			   (xml, "contacts_treeview"));
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW
						     (contacts_treeview),
						     -1, NULL, renderer,
						     "text", CONTACT_NAME_COL,
						     NULL);
	/* Create model and groups/search filter for contacts list */
	model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	filter =
	    GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new
				   (GTK_TREE_MODEL (model), NULL));
	gtk_tree_model_filter_set_visible_func (filter,
						(GtkTreeModelFilterVisibleFunc)
						 contacts_is_row_visible_cb,
						contacts_data->contacts_table,
						NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (contacts_treeview),
				 GTK_TREE_MODEL (filter));
	/* Alphabetise the list */
	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (model),
						 contacts_sort_treeview_cb,
						 NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
				      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
				      GTK_SORT_ASCENDING);
	g_object_unref (model);
	/* Connect signal for selection changed event */
	selection = gtk_tree_view_get_selection (contacts_treeview);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (contacts_selection_cb), contacts_data);
	
	/* Create model/view for groups/type chooser list */
	contacts_treeview = GTK_TREE_VIEW (glade_xml_get_widget (
						xml, "chooser_treeview"));
	model = gtk_list_store_new (2, G_TYPE_BOOLEAN, G_TYPE_STRING);
	renderer = gtk_cell_renderer_toggle_new ();
	g_object_set (renderer, "activatable", TRUE, NULL);
	g_signal_connect (renderer, "toggled",
			  (GCallback) chooser_toggle_cb, model);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW
						     (contacts_treeview), -1,
						     NULL, renderer, "active",
						     CHOOSER_TICK_COL, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW
						     (contacts_treeview), -1,
						     NULL, renderer, "text",
						     CHOOSER_NAME_COL, NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (contacts_treeview),
				 GTK_TREE_MODEL (model));
	g_object_unref (model);

	/* Select 'All' in the groups combobox */
	groups_combobox = GTK_COMBO_BOX (glade_xml_get_widget 
					 (xml, "groups_combobox"));
	gtk_combo_box_set_active (groups_combobox, 0);

	/* Set transient parent for chooser and about dialog */
	gtk_window_set_transient_for (
		GTK_WINDOW (glade_xml_get_widget (xml, "chooser_dialog")),
		GTK_WINDOW (glade_xml_get_widget (xml, "main_window")));
	gtk_window_set_transient_for (
		GTK_WINDOW (glade_xml_get_widget (xml, "about_dialog")),
		GTK_WINDOW (glade_xml_get_widget (xml, "main_window")));
	
	/* Set can_focus to FALSE for summary name label & contacts treeview */
	/* FIXME: This breaks cut/copy/paste, think of a better solution */
/*	g_value_init (can_focus, G_TYPE_BOOLEAN);
	g_value_set_boolean (can_focus, FALSE);
	widget = glade_xml_get_widget (xml, "summary_name_label");
	g_object_set_property (G_OBJECT (widget), "can-focus", can_focus);
	g_value_unset (can_focus);
	g_free (can_focus);*/
	widget = glade_xml_get_widget (xml, "preview_header_hbox");
	contacts_remove_labels_from_focus_chain (GTK_CONTAINER (widget));

	/* Connect UI-related signals */
	widget = glade_xml_get_widget (xml, "new_button");
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (contacts_new_cb), contacts_data);
	widget = glade_xml_get_widget (xml, "new");
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (contacts_new_cb), contacts_data);
	widget = glade_xml_get_widget (xml, "edit_button");
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (contacts_edit_cb), contacts_data);
	widget = glade_xml_get_widget (xml, "contacts_treeview");
	g_signal_connect (G_OBJECT (widget), "row_activated",
			  G_CALLBACK (contacts_treeview_edit_cb), contacts_data);
	widget = glade_xml_get_widget (xml, "edit");
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (contacts_edit_cb), contacts_data);
	widget = glade_xml_get_widget (xml, "delete_button");
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (contacts_delete_cb), contacts_data);
	widget = glade_xml_get_widget (xml, "delete");
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (contacts_delete_cb), contacts_data);
	widget = glade_xml_get_widget (xml, "contacts_import");
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (contacts_import_cb), contacts_data);

			  
	/* Start */
	g_idle_add (open_book, contacts_data);
	gtk_main ();

	/* Unload the addressbook */
	e_book_view_stop (contacts_data->book_view);
	g_object_unref (contacts_data->book_view);
	g_object_unref (contacts_data->book);

	return 0;
}
