/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME indent:  indent -kr -i8 -pcs -lps -psl */

#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libebook/e-book.h>

#include "defs.h"
#include "utils.h"
#include "callbacks-ui.h"
#include "callbacks-ebook.h"

#define XML_FILE PKGDATADIR "/contacts.glade"

GladeXML *xml;
GList *contacts_groups;
GHashTable *contacts_table;
EBook *book;
EBookView *book_view;


void
contacts_main_quit ()
{
	/* Unload the addressbook and quit */
	e_book_view_stop (book_view);
	g_object_unref (book_view);
	g_object_unref (book);
	gtk_main_quit ();
}


void
contacts_update_treeview ()
{
	GtkTreeView *view;
	GtkTreeModelFilter *model;
	gint visible_rows;

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

/* Shows GtkLabel widgets label_name and name and sets the text of name to
 * string. If string is NULL, hides both widgets
 */
static void
contacts_set_label (const gchar * label_name, const gchar * name,
	   const gchar * string)
{
	GtkWidget *label, *title_label;

	label = glade_xml_get_widget (xml, name);
	title_label = glade_xml_get_widget (xml, label_name);

	if (label && title_label) {
		if (string) {
			gtk_label_set_text (GTK_LABEL (label), string);
			gtk_widget_show (title_label);
			gtk_widget_show (label);
		} else {
			gtk_widget_hide (title_label);
			gtk_widget_hide (label);;
		}
	}
}

void
contacts_display_summary (EContact *contact)
{
	GtkWidget *widget;
	const gchar *string;
	GtkImage *photo;

	if (!E_IS_CONTACT (contact))
		return;

	/* Retrieve contact name */
	widget = glade_xml_get_widget (xml, "summary_name_label");
	string = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
	if (string) {
		gchar *name_markup = g_strdup_printf
		    ("<span><big><b>%s</b></big></span>", string);
		gtk_label_set_markup (GTK_LABEL (widget), name_markup);
		g_free (name_markup);
	} else {
		gtk_label_set_markup (GTK_LABEL (widget), "");
	}

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

	/* Retrieve contact business phone number */
	string = e_contact_get_const (contact, E_CONTACT_PHONE_BUSINESS);
	contacts_set_label ("bphone_title_label", "bphone_label", string);

	/* Retrieve contact home phone number */
	string = e_contact_get_const (contact, E_CONTACT_PHONE_HOME);
	contacts_set_label ("hphone_title_label", "hphone_label", string);

	/* Retrieve contact mobile phone number */
	string = e_contact_get_const (contact, E_CONTACT_PHONE_MOBILE);
	contacts_set_label ("mobile_title_label", "mobile_label", string);

	/* Retrieve contact e-mail address */
	string = e_contact_get_const (contact, E_CONTACT_EMAIL_1);
	contacts_set_label ("email_title_label", "email_label", string);

	/* Retrieve contact address */
	string = e_contact_get_const (contact, E_CONTACT_ADDRESS_LABEL_HOME);
	contacts_set_label ("address_title_label", "address_label", string);

	widget = glade_xml_get_widget (xml, "summary_vbox");
	gtk_widget_show (widget);
	contact_selected_sensitive (TRUE);
}

int
main (int argc, char **argv)
{
	GtkComboBox *groups_combobox;
	GtkTreeView *contacts_treeview;
	GtkTreeSelection *selection;
	GtkListStore *model;
	GtkTreeModelFilter *filter;
	GtkCellRenderer *renderer;
	gboolean status;
	EBookQuery *query;

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

	/* Hook up signals defined in interface xml */
	glade_xml_signal_autoconnect (xml);

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
						contacts_is_row_visible_cb,
						NULL, NULL);
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
			  G_CALLBACK (contacts_selection_cb), NULL);
	
	/* Add the columns to the groups/type chooser list */
	contacts_treeview = GTK_TREE_VIEW (glade_xml_get_widget (
						xml, "types_treeview"));
	renderer = gtk_cell_renderer_toggle_new ();
	g_object_set (renderer, "activatable", TRUE, NULL);
/*	g_signal_connect (renderer, "toggled",
			  (GCallback) contacts_type_chooser_toggle_cb, NULL);*/
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW
						     (contacts_treeview), -1,
						     NULL, renderer, "active",
						     TYPE_CHOSEN_COL, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW
						     (contacts_treeview), -1,
						     NULL, renderer, "text",
						     TYPE_NAME_COL, NULL);
	/* Create model for groups/type chooser list */
	model = gtk_list_store_new (2, G_TYPE_BOOLEAN, G_TYPE_STRING);

	/* Select 'All' in the groups combobox */
	groups_combobox = GTK_COMBO_BOX (glade_xml_get_widget 
					 (xml, "groups_combobox"));
	gtk_combo_box_set_active (groups_combobox, 0);
	contacts_groups = NULL;

	/* Load the system addressbook */
	contacts_table = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, 
						(GDestroyNotify)
						 contacts_free_list_hash);
	book = e_book_new_system_addressbook (NULL);
	if (!book)
		g_critical ("Could not load system addressbook");
	if (!(status = e_book_open (book, FALSE, NULL)))
		g_critical ("Error '%d' opening ebook", status);
	/* Create an EBookView */
	query = e_book_query_any_field_contains ("");
	e_book_get_book_view (book, query, NULL, -1, &book_view, NULL);
	e_book_query_unref (query);
	/* Connect signals */
	g_signal_connect (G_OBJECT (book_view), "contacts_added",
			  G_CALLBACK (contacts_added_cb), NULL);
	g_signal_connect (G_OBJECT (book_view), "contacts_changed",
			  G_CALLBACK (contacts_changed_cb), NULL);
	g_signal_connect (G_OBJECT (book_view), "contacts_removed",
			  G_CALLBACK (contacts_removed_cb), NULL);
	/* Start */
	e_book_view_start (book_view);

	gtk_main ();

	return 0;
}
