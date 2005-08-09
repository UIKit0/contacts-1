/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME indent:  indent -kr -i8 -pcs -lps -psl */

#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libebook/e-book.h>

#define XML_FILE "../data/contacts.glade" /* FIXME: pull in via PKGDATADIR */

static GladeXML *xml;
static EBook *book;
static GList *cards;
static GtkComboBox *groups_combobox;
static gint groups_size;

/* List of always-available fields that are one line */
static EContactField base_fields1[] = {
	E_CONTACT_FULL_NAME,
	E_CONTACT_PHONE_BUSINESS,
	E_CONTACT_PHONE_HOME,
	E_CONTACT_PHONE_MOBILE,
	E_CONTACT_EMAIL_1,
	E_CONTACT_FIELD_LAST
};
/* List of always-available fields that span multiple lines (addresses) */
static EContactField base_fieldsn[] = {
	E_CONTACT_ADDRESS_LABEL_HOME,
	E_CONTACT_ADDRESS_LABEL_WORK,
	E_CONTACT_FIELD_LAST
};

typedef enum {
	NAME,
	ECONTACT,
	LAST
} COLUMNS;

/* The following functions taken from
 * http://svn.o-hand.com/repos/kozo/server/src/kozo-utf8.c
 */
 /*****************************************************************************/
static char *
e_util_unicode_get_utf8 (const char *text, gunichar * out)
{
	*out = g_utf8_get_char (text);

	return (G_UNLIKELY (*out == (gunichar) - 1)) ? NULL :
	    g_utf8_next_char (text);
}

static gunichar
stripped_char (gunichar ch)
{
	GUnicodeType utype;

	utype = g_unichar_type (ch);

	switch (utype) {
	case G_UNICODE_CONTROL:
	case G_UNICODE_FORMAT:
	case G_UNICODE_UNASSIGNED:
	case G_UNICODE_COMBINING_MARK:
		/* Ignore those */
		return 0;
	default:
		/* Convert to lowercase, fall through */
		ch = g_unichar_tolower (ch);
	case G_UNICODE_LOWERCASE_LETTER:
		return ch;
	}
}

static const char *
kozo_utf8_strstrcasestrip (const char *haystack, const gunichar * needle)
{
	gunichar unival, sc;
	const char *o, *p, *q;
	int npos;

	/* FIXME this is unoptimal */
	/* TODO use Bayer-Moore algorithm, if faster */
	o = haystack;
	for (p = e_util_unicode_get_utf8 (o, &unival);
	     p && unival; p = e_util_unicode_get_utf8 (p, &unival)) {
		sc = stripped_char (unival);
		if (sc) {
			/* We have valid stripped char */
			if (sc == needle[0]) {
				q = p;
				npos = 1;

				while (needle[npos]) {
					q = e_util_unicode_get_utf8 (q,
								     &unival);
					if (!q || !unival)
						return NULL;

					sc = stripped_char (unival);
					if ((!sc) || (sc != needle[npos]))
						break;

					npos++;
				}

				if (!needle[npos])
					return o;
			}
		}

		o = p;
	}

	return NULL;
}

static gunichar *
kozo_utf8_strcasestrip (const char *str)
{
	gunichar *needle;
	gunichar unival, sc;
	int nlen, normlen;
	const char *p;
	char *norm;

	/* FIXME unoptimal, too many iterations */
	norm = g_utf8_normalize (str, -1, G_NORMALIZE_DEFAULT);
	normlen = g_utf8_strlen (norm, -1);
	if (G_UNLIKELY (normlen == 0)) {
		g_free (norm);
		return NULL;
	}

	/* Over-estimates length */
	needle = g_new (gunichar, normlen + 1);

	nlen = 0;
	for (p = e_util_unicode_get_utf8 (norm, &unival);
	     p && unival; p = e_util_unicode_get_utf8 (p, &unival)) {
		sc = stripped_char (unival);
		if (sc)
			needle[nlen++] = sc;
	}

	g_free (norm);

	/* NULL means there was illegal utf-8 sequence */
	if (!p)
		return NULL;

	/* If everything is correct, we have decomposed, lowercase, stripped 
	 * needle */
	needle[nlen] = 0;
	if (normlen == nlen)
		return needle;
	else
		return g_renew (gunichar, needle, nlen + 1);
}

/******************************************************************************/

static void
free_object_list (GList * list)
{
	if (list) {
		g_list_foreach (list, (GFunc) g_object_unref, NULL);
		g_list_free (list);
		list = NULL;
	}
}

void
quit ()
{
	/* Unload the addressbook and quit */
	free_object_list (cards);
	g_object_unref (book);
	gtk_main_quit ();
}

static gboolean
is_row_visible (GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	gboolean result = FALSE;
	gchar *group = gtk_combo_box_get_active_text (groups_combobox);
	GList *groups, *g;
	EContact *contact;
	const gchar *search_string = gtk_entry_get_text (GTK_ENTRY
							 (glade_xml_get_widget
							  (xml,
							   "search_entry")));

	/* Check if the contact is in the currently selected group. */
	gtk_tree_model_get (model, iter, ECONTACT, &contact, -1);
	groups = e_contact_get (contact, E_CONTACT_CATEGORY_LIST);
	if (gtk_combo_box_get_active (groups_combobox) > 0) {
		for (g = groups; g; g = g->next) {
			if (strcmp (group, g->data) == 0)
				result = TRUE;
			g_free (g->data);
		}
		if (groups)
			g_list_free (groups);
		if (!result)
			return FALSE;
	} else
		result = TRUE;

	/* Search for any occurrence of the string in the search box in the 
	 * contact file-as name; if none is found, row isn't visible. Ignores 
	 * empty searches.
	 */
	if ((search_string) && (search_string[0] != '\0')) {
		gchar *name_string;
		gtk_tree_model_get (model, iter, NAME, &name_string, -1);
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
sort_treeview_func (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b,
		    gpointer user_data)
{
	gchar *string1, *string2;
	gint returnval;

	gtk_tree_model_get (model, a, NAME, &string1, -1);
	gtk_tree_model_get (model, b, NAME, &string2, -1);
	returnval =
	    strcasecmp ((const char *) string1, (const char *) string2);
	g_free (string1);
	g_free (string2);

	return returnval;
}

void
update_treeview ()
{
	GtkTreeModelFilter *model;

	model =
	    GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model
				   (GTK_TREE_VIEW
				    (glade_xml_get_widget
				     (xml, "contacts_treeview"))));
	gtk_tree_model_filter_refilter (model);
}

static void
fill_treeview (GtkListStore * model)
{
	EBookQuery *query;
	gboolean status;
	GList *c;

	/* Create query */
	query = e_book_query_field_exists (E_CONTACT_FULL_NAME);

	/* Retrieve contacts */
	/* TODO: Use e_book_get_book_view */
	free_object_list (cards);
	status = e_book_get_contacts (book, query, &cards, NULL);

	/* Free query structure */
	e_book_query_unref (query);

	if (status == FALSE) {
		g_warning ("Error '%d' retrieving contacts", status);
		return;
	} else {
		gint i;
		GList *groups = NULL;

		/* Clear current contacts list and group list */
		for (i = 0; i < groups_size; i++)
			gtk_combo_box_remove_text (groups_combobox, 1);
		gtk_list_store_clear (model);

		/* Iterate over results and add them to the list */
		for (c = cards; c; c = c->next) {
			GtkTreeIter iter;
			const gchar *file_as;
			GList *contact_groups;
			EContact *contact = E_CONTACT (c->data);

			/* Add contact to list */
			file_as =
			    e_contact_get_const (contact,
						 E_CONTACT_FULL_NAME);
			gtk_list_store_append (model, &iter);
			gtk_list_store_set (model, &iter, NAME, file_as,
					    ECONTACT, contact, -1);

			/* Check for groups and add them to group list */
			contact_groups =
			    e_contact_get (contact,
					   E_CONTACT_CATEGORY_LIST);
			if (contact_groups) {
				GList *group;
				for (group = contact_groups; group;
				     group = group->next) {
					if (!g_list_find_custom
					    (groups, group->data,
					     (GCompareFunc) strcmp)) {
						gtk_combo_box_append_text
						    (groups_combobox,
						     group->data);
						groups =
						    g_list_prepend (groups,
								    group->
								    data);
					}
				}
				g_list_free (contact_groups);
			}
		}
		g_list_foreach (groups, (GFunc) g_free, NULL);
		g_list_free (groups);
	}
}

/* Shows GtkLabel widgets label_name and name and sets the text of name to
 * string. If string is NULL, hides both widgets
 */
static void
set_label (const gchar * label_name, const gchar * name,
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


/* Helper method to set edit/delete sensitive/insensitive */
static void
contact_selected_sensitive (gboolean sensitive)
{
	GtkWidget *widget;

	widget = glade_xml_get_widget (xml, "edit");
	gtk_widget_set_sensitive (widget, sensitive);
	widget = glade_xml_get_widget (xml, "edit_button");
	gtk_widget_set_sensitive (widget, sensitive);

	widget = glade_xml_get_widget (xml, "delete");
	gtk_widget_set_sensitive (widget, sensitive);
	widget = glade_xml_get_widget (xml, "delete_button");
	gtk_widget_set_sensitive (widget, sensitive);
}

static void
contact_photo_size (GdkPixbufLoader * loader, gint width, gint height,
		    gpointer user_data)
{
	/* Max height of GTK_ICON_SIZE_DIALOG */
	gint iconwidth, iconheight;
	gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &iconwidth, &iconheight);
	
	gdk_pixbuf_loader_set_size (loader,
				    width / ((gdouble) height /
					     iconheight), iconheight);
}
static GtkImage *
load_contact_photo (EContact *contact)
{
	GtkImage *image = NULL;
	EContactPhoto *photo;
	
	/* Retrieve contact picture and resize */
	/* TODO: Am I leaking memory here? */
	photo = e_contact_get (contact, E_CONTACT_PHOTO);
	if (photo) {
		GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
		g_signal_connect (G_OBJECT (loader),
				  "size-prepared",
				  G_CALLBACK (contact_photo_size),
				  NULL);
		if ((gdk_pixbuf_loader_write
		     (loader, photo->data, photo->length, NULL))
		    && (gdk_pixbuf_loader_close (loader, NULL))) {
			GdkPixbuf *pixbuf =
			    gdk_pixbuf_loader_get_pixbuf (loader);
			if (pixbuf) {
				image = GTK_IMAGE (gtk_image_new_from_pixbuf
						   (pixbuf));
			}
		}
		g_object_unref (loader);
		e_contact_photo_free (photo);
	}
	return image ? image : GTK_IMAGE (gtk_image_new_from_icon_name 
					("stock_person", GTK_ICON_SIZE_DIALOG));
}

static void
contact_selected (GtkTreeSelection * selection)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkWidget *widget;
	GtkImage *photo;
	const gchar *string;
	EContact *contact;

	/* Get the currently selected contact and update the contact summary */
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, ECONTACT, &contact, -1);

		/* Retrieve contact name */
		string = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
		if (string) {
			gchar *name_markup = g_strdup_printf
			    ("<span><big><b>%s</b></big></span>", string);
			widget =
			    glade_xml_get_widget (xml,
						  "summary_name_label");
			gtk_label_set_markup (GTK_LABEL (widget),
					      name_markup);
			g_free (name_markup);
		}

		/* Retrieve contact picture and resize */
		widget = glade_xml_get_widget (xml, "photo_image");
		photo = load_contact_photo (contact);
		if ((gtk_image_get_storage_type (photo) == GTK_IMAGE_EMPTY) ||
		    (gtk_image_get_storage_type (photo) == GTK_IMAGE_PIXBUF))
			gtk_image_set_from_pixbuf (GTK_IMAGE (widget),
						  gtk_image_get_pixbuf (photo));
		else if (gtk_image_get_storage_type 
			 (photo) == GTK_IMAGE_ICON_NAME) {
			const gchar *icon_name;
			GtkIconSize size;
			gtk_image_get_icon_name (photo, &icon_name, 
						 &size);
			gtk_image_set_from_icon_name (GTK_IMAGE (widget),
						      icon_name,
						      size);
		}
		gtk_widget_destroy (GTK_WIDGET (photo));

		/* Retrieve contact business phone number */
		string =
		    e_contact_get_const (contact,
					 E_CONTACT_PHONE_BUSINESS);
		set_label ("bphone_title_label", "bphone_label", string);

		/* Retrieve contact home phone number */
		string =
		    e_contact_get_const (contact, E_CONTACT_PHONE_HOME);
		set_label ("hphone_title_label", "hphone_label", string);

		/* Retrieve contact mobile phone number */
		string =
		    e_contact_get_const (contact, E_CONTACT_PHONE_MOBILE);
		set_label ("mobile_title_label", "mobile_label", string);

		/* Retrieve contact e-mail address */
		string = e_contact_get_const (contact, E_CONTACT_EMAIL_1);
		set_label ("email_title_label", "email_label", string);

		/* Retrieve contact address */
		string =
		    e_contact_get_const (contact,
					 E_CONTACT_ADDRESS_LABEL_HOME);
		set_label ("address_title_label", "address_label", string);

		widget = glade_xml_get_widget (xml, "summary_vbox");
		gtk_widget_show (widget);
		contact_selected_sensitive (TRUE);
	} else {
		contact_selected_sensitive (FALSE);
		widget = glade_xml_get_widget (xml, "summary_vbox");
		gtk_widget_hide (widget);
	}
}

typedef struct {
	EContact *contact;
	EContactField field_id;
} EContactChangeData;

static void
text_entry_changed (GtkEntry *entry, gpointer data)
{
	gchar *string;
	EContactChangeData *d = (EContactChangeData *)data;
	
	string = (gchar *)gtk_entry_get_text (entry);
	if (g_utf8_strlen (string, -1) == 0)
		string = NULL;
	e_contact_set (d->contact, d->field_id, (gpointer)string);
}

static void
free_change_data (GtkEntry *entry, gpointer data)
{
	if (data)
		g_free (data);
}

/* This function adds a GtkLabel and GtkEntry derived from field_id for a
 * particular contact to a GtkTable of 2 columns, with the specified row.
 * Returns TRUE on success, FALSE on failure.
 */
static gboolean
static_field_contact_edit_add (EContact *contact, EContactField field_id, 
			       GtkTable *table, guint row, GCallback cb)
{
	GtkWidget *label;
	GtkWidget *entry;
	const gchar *string;
	EContactChangeData *data;
	
	string = e_contact_get_const (contact, field_id);
	if (!string)
		string = "";
	
	/* Create widgets */
	label = gtk_label_new (e_contact_pretty_name (field_id));
	entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry), string);
	
	/* Set alignment and add to container */
	gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
	gtk_table_attach (table, label, 0, 1, row, row+1, GTK_FILL, GTK_FILL, 
			  0, 0);
	gtk_table_attach (table, entry, 1, 2, row, row+1, GTK_FILL | GTK_EXPAND,
			  GTK_FILL, 0, 0);
	
	/* Connect signal */
	data = g_new (EContactChangeData, 1);
	data->contact = contact;
	data->field_id = field_id;
	g_signal_connect (G_OBJECT (entry), "changed", cb, data);
	g_signal_connect (G_OBJECT (entry), "destroy", G_CALLBACK 
			  (free_change_data), data);
	
	/* Display widgets */
	gtk_widget_show (label);
	gtk_widget_show (entry);
	
	return TRUE;
}

static void
do_edit (EContact *contact)
{
	GtkWidget *widget;
	guint i, row;
	
	/* Display contact photo */
	widget = glade_xml_get_widget (xml, "edit_photo_button");
	gtk_button_set_image (GTK_BUTTON (widget), 
			      GTK_WIDGET (load_contact_photo (contact)));
	
	/* Fill edit pane */
	/* Always-available fields */
	widget = glade_xml_get_widget (xml, "edit_table");
	for (i = 0, row = 0; base_fields1[i] != E_CONTACT_FIELD_LAST; i++) {
		if (static_field_contact_edit_add (contact, base_fields1[i],
						   GTK_TABLE (widget), row, 
						   G_CALLBACK 
						   (text_entry_changed)))
			row++;
	}
	
	/* Custom fields */
	/* ... */
	
	/* Display edit pane */
	widget = glade_xml_get_widget (xml, "main_notebook");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 1);
	
	widget = glade_xml_get_widget (xml, "main_window");
	gtk_window_set_title (GTK_WINDOW (widget), "Edit contact");
}

void
new_contact ()
{
}

void
edit_contact ()
{
	GtkWidget *widget;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	EContact *contact;

	contact_selected_sensitive (FALSE);
	
	/* Get the contact to edit */
	widget = glade_xml_get_widget (xml, "contacts_treeview");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	gtk_tree_selection_get_selected (selection, &model, &iter);
	gtk_tree_model_get (model, &iter, ECONTACT, &contact, -1);
	
	do_edit (contact);
}

static void
remove_edit_components_cb (GtkWidget *widget, gpointer data)
{
	gtk_container_remove (GTK_CONTAINER (data), widget);
}

void
edit_done ()
{
	GtkWidget *widget;
	
	/* All changed are instant-apply, so just remove the edit components
	 * and switch back to main view.
	 */
	widget = glade_xml_get_widget (xml, "edit_table");
	gtk_container_foreach (GTK_CONTAINER (widget),
			       (GtkCallback)remove_edit_components_cb, widget);
	widget = glade_xml_get_widget (xml, "main_notebook");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 0);
	widget = glade_xml_get_widget (xml, "main_window");
	gtk_window_set_title (GTK_WINDOW (widget), "Contacts");
	contact_selected_sensitive (TRUE);
}

void
delete_contact ()
{
}

int
main (int argc, char **argv)
{
	GtkVBox *contacts_vbox;
	GtkTreeView *contacts_treeview;
	GtkTreeSelection *selection;
	GtkListStore *model;
	GtkTreeModelFilter *filter;
	gchar *book_path;
	GtkCellRenderer *renderer;
	gboolean status;

	/* Standard initialisation for gtk and glade */
	gtk_init (&argc, &argv);
	glade_init ();

	cards = NULL;

	/* Set critical errors to close application */
	g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);

	/* Load up main_window from interface xml, quit if xml file not found */
	xml = glade_xml_new (XML_FILE, "main_window", NULL);
	if (!xml)
		g_critical ("Could not find interface XML file '%s'",
			    XML_FILE);

	/* Hook up signals defined in interface xml */
	glade_xml_signal_autoconnect (xml);

	/* Load the system addressbook */
	book_path = g_strdup_printf
	    ("file://%s/.evolution/addressbook/local/system/",
	     g_get_home_dir ());
	book = e_book_new_from_uri (book_path, NULL);
	g_free (book_path);
	if (!book)
		g_critical ("Could not load system addressbook");
	status = e_book_open (book, FALSE, NULL);
	if (status == FALSE)
		g_critical ("Error '%d' opening ebook", status);

	/* Create the groups combobox */
	groups_combobox = GTK_COMBO_BOX (gtk_combo_box_new_text ());
	/* Add virtual group 'All' */
	gtk_combo_box_append_text (groups_combobox, "All");
	gtk_combo_box_set_active (groups_combobox, 0);
	groups_size = 0;
	/* Pack groups combobox into contacts_vbox */
	contacts_vbox =
	    GTK_VBOX (glade_xml_get_widget (xml, "contacts_vbox"));
	gtk_box_pack_start (GTK_BOX (contacts_vbox),
			    GTK_WIDGET (groups_combobox), FALSE, TRUE, 6);
	gtk_widget_show (GTK_WIDGET (groups_combobox));
	g_signal_connect (G_OBJECT (groups_combobox), "changed",
			  G_CALLBACK (update_treeview), NULL);

	/* Add the column to the GtkTreeView */
	contacts_treeview =
	    GTK_TREE_VIEW (glade_xml_get_widget
			   (xml, "contacts_treeview"));
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW
						     (contacts_treeview),
						     -1, NULL, renderer,
						     "text", NAME, NULL);
	/* Create model and fill model */
	model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
	fill_treeview (model);
	filter =
	    GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new
				   (GTK_TREE_MODEL (model), NULL));
	gtk_tree_model_filter_set_visible_func (filter, is_row_visible,
						NULL, NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (contacts_treeview),
				 GTK_TREE_MODEL (filter));
	/* Alphabetise the GtkTreeView */
	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (model),
						 sort_treeview_func, NULL,
						 NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
					      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					      GTK_SORT_ASCENDING);

	g_object_unref (model);

	/* Connect signal for selection changed event */
	selection = gtk_tree_view_get_selection (contacts_treeview);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (contact_selected), NULL);

	gtk_main ();

	return 0;
}
