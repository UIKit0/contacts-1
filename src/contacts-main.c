/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME indent:  indent -kr -i8 -pcs -lps -psl */

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
contacts_display_summary (EContact *contact, GladeXML *xml)
{
	GtkWidget *widget;
	const gchar *string;
	GtkImage *photo;
	GList *a, *groups, *attributes;
	gchar *name_markup, *groups_text = NULL;

	if (!E_IS_CONTACT (contact))
		return;

	/* Retrieve contact name and groups */
	widget = glade_xml_get_widget (xml, "summary_name_label");
	string = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
	groups = e_contact_get (contact, E_CONTACT_CATEGORY_LIST);
	groups_text = contacts_string_list_as_string (groups, ", ");
	name_markup = g_strdup_printf
		("<span><big><b>%s</b></big>\n<small>%s</small></span>",
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
	attributes = e_vcard_get_attributes (&contact->parent);
	for (a = attributes; a; a = a->next) {
		GtkWidget *name_widget, *value_widget;
		gchar *value_text, *name_markup;
		GList *values;
		const gchar **types;
		EVCardAttribute *attr = (EVCardAttribute *)a->data;
		const ContactsField *field = contacts_get_contacts_field (
			e_vcard_attribute_get_name (attr));
			
		if (!field || field->priority >= REQUIRED)
			continue;
		
		values = e_vcard_attribute_get_values (attr);
		value_text = contacts_string_list_as_string (values, "\n");
		
		types = contacts_get_field_types (e_vcard_attribute_get_name (attr));
		if (types) {
			gchar *types_string;
			GList *types_list = NULL;

			types_list = contacts_get_type_strings (
				e_vcard_attribute_get_params (attr));
			types_string = types_list ?
			    contacts_string_list_as_string (types_list, ", ") :
			    g_strdup("Other");
			g_list_free (types_list);
			
			name_markup = g_strdup_printf (
				"<span><b>%s:</b>\n<small>(%s)</small></span>",
				contacts_field_pretty_name (field),
				types_string);
			g_free (types_string);
		} else {
			name_markup = g_strdup_printf ("<span><b>%s:</b></span>",
				contacts_field_pretty_name (field));
		}
		name_widget = gtk_label_new (name_markup);
		gtk_label_set_use_markup (GTK_LABEL (name_widget), TRUE);
		value_widget = gtk_label_new (value_text);
		gtk_label_set_justify (GTK_LABEL (name_widget),
				       GTK_JUSTIFY_RIGHT);
		gtk_misc_set_alignment (GTK_MISC (name_widget), 1, 0);
		gtk_misc_set_alignment (GTK_MISC (value_widget), 0, 0);
		
		contacts_append_to_edit_table (GTK_TABLE (widget), name_widget,
					       value_widget);
		
		g_free (name_markup);
		g_free (value_text);
	}

	widget = glade_xml_get_widget (xml, "summary_vbox");
	gtk_widget_show (widget);
	contacts_set_available_options (xml, TRUE, TRUE, TRUE);
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

int
main (int argc, char **argv)
{
	GtkWidget *widget;		/* Variables for UI initialisation */
	GtkComboBox *groups_combobox;
	GtkTreeView *contacts_treeview;
	GtkTreeSelection *selection;
	GtkListStore *model;
	GtkTreeModelFilter *filter;
	GtkCellRenderer *renderer;
	gboolean status;
	GladeXML *xml;			/* */
	EBook *book;			/* EBook variables */
	EBookView *book_view;
	EBookQuery *query;		/* */
	ContactsData *contacts_data;	/* Variable for passing around data -
					 * see contacts-defs.h.
					 */

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
	book = e_book_new_system_addressbook (NULL);
	if (!book)
		g_critical ("Could not load system addressbook");
	if (!(status = e_book_open (book, FALSE, NULL)))
		g_critical ("Error '%d' opening ebook", status);
	/* Create an EBookView */
	query = e_book_query_any_field_contains ("");
	e_book_get_book_view (book, query, NULL, -1, &book_view, NULL);
	e_book_query_unref (query);

	/* Hook up signals defined in interface xml */
	glade_xml_signal_autoconnect (xml);
	
	/* Initialise global data construct */
	contacts_data = g_new0 (ContactsData, 1);
	contacts_data->book = book;
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
	widget = glade_xml_get_widget (xml, "edit");
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (contacts_edit_cb), contacts_data);
	widget = glade_xml_get_widget (xml, "delete_button");
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (contacts_delete_cb), contacts_data);
	widget = glade_xml_get_widget (xml, "delete");
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (contacts_delete_cb), contacts_data);

	/* Connect signals on EBookView */
	g_signal_connect (G_OBJECT (book_view), "contacts_added",
			  G_CALLBACK (contacts_added_cb), contacts_data);
	g_signal_connect (G_OBJECT (book_view), "contacts_changed",
			  G_CALLBACK (contacts_changed_cb), contacts_data);
	g_signal_connect (G_OBJECT (book_view), "contacts_removed",
			  G_CALLBACK (contacts_removed_cb), contacts_data);
			  
	/* Start */
	e_book_view_start (book_view);
	gtk_main ();

	/* Unload the addressbook */
	e_book_view_stop (book_view);
	g_object_unref (book_view);
	g_object_unref (book);

	return 0;
}
