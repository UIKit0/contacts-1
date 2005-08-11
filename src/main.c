/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME indent:  indent -kr -i8 -pcs -lps -psl */

#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libebook/e-book.h>

#define XML_FILE PKGDATADIR "/contacts.glade"

static GladeXML *xml;
static GList *contacts_groups;
static GHashTable *contacts_table;
static EBook *book;
static EBookView *book_view;

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
	UID,
	LAST
} COLUMNS;

typedef struct {
	EContact *contact;
	GtkTreeIter iter;
} EContactListHash;

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
	e_book_view_stop (book_view);
	g_object_unref (book_view);
	g_object_unref (book);
	gtk_main_quit ();
}

static void
free_list_hash (gpointer data)
{
	EContactListHash *hash = (EContactListHash *)data;
	
	if (hash) {
		GtkListStore *model = GTK_LIST_STORE 
			(gtk_tree_model_filter_get_model 
			 (GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model 
			  (GTK_TREE_VIEW (glade_xml_get_widget
					(xml, "contacts_treeview"))))));
		gtk_list_store_remove (model, &hash->iter);
		g_object_unref (hash->contact);
		g_free (hash);
	}
}

static gboolean
is_row_visible (GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	gboolean result = FALSE;
	gchar *group;
	GList *groups, *g;
	const gchar *uid;
	EContactListHash *hash;
	GtkComboBox *groups_combobox;
	const gchar *search_string = gtk_entry_get_text (GTK_ENTRY
							 (glade_xml_get_widget
							  (xml,
							   "search_entry")));

	/* Check if the contact is in the currently selected group. */
	gtk_tree_model_get (model, iter, UID, &uid, -1);
	if (!uid) return FALSE;
	hash = g_hash_table_lookup (contacts_table, uid);
	if (!hash || !hash->contact) return FALSE;
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

static EContact *
get_contact_from_selection (GtkTreeSelection *selection)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	if (!selection || !GTK_IS_TREE_SELECTION (selection))
		return NULL;
		
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		const gchar *uid;
		EContactListHash *hash;
		gtk_tree_model_get (model, &iter, UID, &uid, -1);
		if (uid) {
			hash = g_hash_table_lookup (contacts_table, uid);
			if (hash)
				return hash->contact;
		}
	}
	return NULL;
}

static EContact *
get_current_contact ()
{
	GtkWidget *widget;
	GtkTreeSelection *selection;
	EContact *contact;

	/* Get the currently selected contact */
	widget = glade_xml_get_widget (xml, "contacts_treeview");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	
	if (!selection || !GTK_IS_TREE_SELECTION (selection))
		return NULL;
		
	contact = get_contact_from_selection (selection);
	
	return contact;
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
	photo = e_contact_get (contact, E_CONTACT_PHOTO);
	if (photo) {
		GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
		if (loader) {
			g_signal_connect (G_OBJECT (loader),
					  "size-prepared",
					  G_CALLBACK (contact_photo_size),
					  NULL);
			gdk_pixbuf_loader_write
				     (loader, photo->data, photo->length, NULL);
			gdk_pixbuf_loader_close (loader, NULL);
			GdkPixbuf *pixbuf =
			    gdk_pixbuf_loader_get_pixbuf (loader);
			if (pixbuf) {
				image = GTK_IMAGE (gtk_image_new_from_pixbuf
						   (g_object_ref (pixbuf)));
			}
			g_object_unref (loader);
		}
		e_contact_photo_free (photo);
	}
	return image ? image : GTK_IMAGE (gtk_image_new_from_icon_name 
					("stock_person", GTK_ICON_SIZE_DIALOG));
}

static void
display_contact_summary (EContact *contact)
{
	GtkWidget *widget;
	const gchar *string;
	GtkImage *photo;

	if (!E_IS_CONTACT (contact))
		return;

	/* Retrieve contact name */
	string = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
	if (string) {
		gchar *name_markup = g_strdup_printf
		    ("<span><big><b>%s</b></big></span>", string);
		widget = glade_xml_get_widget (xml, "summary_name_label");
		gtk_label_set_markup (GTK_LABEL (widget), name_markup);
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
		gtk_image_get_icon_name (photo, &icon_name, &size);
		gtk_image_set_from_icon_name (GTK_IMAGE (widget), icon_name,
					      size);
	}
	gtk_widget_destroy (GTK_WIDGET (photo));

	/* Retrieve contact business phone number */
	string = e_contact_get_const (contact, E_CONTACT_PHONE_BUSINESS);
	set_label ("bphone_title_label", "bphone_label", string);

	/* Retrieve contact home phone number */
	string = e_contact_get_const (contact, E_CONTACT_PHONE_HOME);
	set_label ("hphone_title_label", "hphone_label", string);

	/* Retrieve contact mobile phone number */
	string = e_contact_get_const (contact, E_CONTACT_PHONE_MOBILE);
	set_label ("mobile_title_label", "mobile_label", string);

	/* Retrieve contact e-mail address */
	string = e_contact_get_const (contact, E_CONTACT_EMAIL_1);
	set_label ("email_title_label", "email_label", string);

	/* Retrieve contact address */
	string = e_contact_get_const (contact, E_CONTACT_ADDRESS_LABEL_HOME);
	set_label ("address_title_label", "address_label", string);

	widget = glade_xml_get_widget (xml, "summary_vbox");
	gtk_widget_show (widget);
	contact_selected_sensitive (TRUE);
}

static void
contacts_added (EBookView *book_view, const GList *contacts)
{
	GtkComboBox *groups_combobox;
	GtkListStore *model;
	GtkTreeModelFilter *filter;
	GList *c;

	/* Get TreeView model and combo box to add contacts/groups */
	filter = GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model 
				 	(GTK_TREE_VIEW (glade_xml_get_widget
						(xml, "contacts_treeview"))));
	model = GTK_LIST_STORE (gtk_tree_model_filter_get_model (filter));
	groups_combobox = GTK_COMBO_BOX (glade_xml_get_widget 
					 (xml, "groups_combobox"));

	/* Iterate over new contacts and add them to the list */
	for (c = (GList *)contacts; c; c = c->next) {
		GList *contact_groups;
		const gchar *name, *uid;
		EContact *contact = E_CONTACT (c->data);
		EContactListHash *hash;
		
		if (!E_IS_CONTACT (contact))
			continue;

		/* Add contact to list */
		hash = g_new (EContactListHash, 1);
		name = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
		uid = e_contact_get_const (contact, E_CONTACT_UID);
		gtk_list_store_insert_with_values (model, &hash->iter, 0,
						   NAME, name,
						   UID, uid,
						   -1);
		hash->contact = g_object_ref (contact);
		g_hash_table_insert (contacts_table, (gchar *)uid, hash);

		/* Check for groups and add them to group list */
		contact_groups =
		    e_contact_get (contact,
				   E_CONTACT_CATEGORY_LIST);
		if (contact_groups) {
			GList *group;
			for (group = contact_groups; group;
			     group = group->next) {
				if (!g_list_find_custom (contacts_groups, 
							 group->data,
							 (GCompareFunc) strcmp))
				{
					gtk_combo_box_append_text
					    (groups_combobox, group->data);
					contacts_groups = g_list_prepend 
							     (contacts_groups,
							      group->data);
				}
			}
			g_list_free (contact_groups);
		}
	}
	
	/* Update view */
	gtk_tree_model_filter_refilter (filter);
}

static void
contacts_changed (EBookView *book_view, const GList *contacts)
{
	GList *c;
	EContact *current_contact = g_object_ref (get_current_contact ());

	/* Loop through changed contacts */	
	for (c = (GList *)contacts; c; c = c->next) {
		EContact *contact = E_CONTACT (c->data);
		EContactListHash *hash;
		const gchar *uid;

		/* Lookup if contact exists in internal list (it should) */
		uid = e_contact_get_const (contact, E_CONTACT_UID);
		hash = g_hash_table_lookup (contacts_table, uid);
		if (!hash) continue;

		/* Replace contact */
		g_object_unref (hash->contact);
		hash->contact = g_object_ref (contact);
		/* TODO: Surely I shouldn't have to do this? */
		g_hash_table_steal (contacts_table, uid);
		g_hash_table_insert (contacts_table, (gchar *)uid, hash);

		/* Update list with possibly new name */
		GtkListStore *model = GTK_LIST_STORE 
			(gtk_tree_model_filter_get_model 
			 (GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model 
			  (GTK_TREE_VIEW (glade_xml_get_widget
					(xml, "contacts_treeview"))))));
		gtk_list_store_set (model, &hash->iter,
			NAME,
			e_contact_get (contact, E_CONTACT_FULL_NAME),
			-1);

		/* If contact is currently selected, update display */
		if (current_contact) {
			if (strcmp (e_contact_get_const
					(contact, E_CONTACT_UID),
				    e_contact_get_const
					(current_contact, E_CONTACT_UID)) == 0)
				display_contact_summary (contact);
		}
	}
	
	g_object_unref (current_contact);
}

static void
contacts_removed (EBookView *book_view, const GList *ids)
{
	GList *i;
	
	for (i = (GList *)ids; i; i = i->next) {
		const gchar *uid = (const gchar *)i->data;
		g_hash_table_remove (contacts_table, uid);
	}
}

static void
contact_selected (GtkTreeSelection * selection)
{
	GtkWidget *widget;
	EContact *contact;

	/* Get the currently selected contact and update the contact summary */
	contact = get_contact_from_selection (selection);
	if (contact) {
		display_contact_summary (contact);
	} else {
		contact_selected_sensitive (FALSE);
		widget = glade_xml_get_widget (xml, "summary_vbox");
		gtk_widget_hide (widget);
	}
}

void
change_photo ()
{
	GtkWidget *widget;
	GtkWidget *filechooser;
	GtkFileFilter *filter;
	gint result;
	
	/* Get a filename */
	widget = glade_xml_get_widget (xml, "main_window");
	filechooser = gtk_file_chooser_dialog_new ("Open image",
						   GTK_WINDOW (widget),
						   GTK_FILE_CHOOSER_ACTION_OPEN,
						   GTK_STOCK_CANCEL,
						   GTK_RESPONSE_CANCEL,
						   GTK_STOCK_OPEN,
						   GTK_RESPONSE_ACCEPT,
						   "No image",
						   GTK_RESPONSE_DELETE_EVENT,
						   NULL);
	/* Set filter by supported EContactPhoto image types */
	filter = gtk_file_filter_new ();
	/* mime types taken from e-contact.c */
	gtk_file_filter_add_mime_type (filter, "image/gif");
	gtk_file_filter_add_mime_type (filter, "image/jpeg");
	gtk_file_filter_add_mime_type (filter, "image/png");
	gtk_file_filter_add_mime_type (filter, "image/tiff");
	gtk_file_filter_add_mime_type (filter, "image/ief");
	gtk_file_filter_add_mime_type (filter, "image/cgm");
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (filechooser), filter);

	/* If a file was selected, get the image and set the contact to that
	 * image.
	 */	
	result = gtk_dialog_run (GTK_DIALOG (filechooser));
	widget = glade_xml_get_widget (xml, "edit_photo_button");
	if (result == GTK_RESPONSE_ACCEPT) {
		gchar *filename = gtk_file_chooser_get_filename 
					(GTK_FILE_CHOOSER (filechooser));
		if (filename) {
			EContact *contact = get_current_contact ();

			if (contact) {
				EContactPhoto new_photo;
				
				if (g_file_get_contents (filename, 
							 &new_photo.data,
							 &new_photo.length,
							 NULL)) {
					e_contact_set (contact, E_CONTACT_PHOTO,
						       &new_photo);
					/* Re-display contact photo */
					gtk_button_set_image (
					   GTK_BUTTON (widget), 
					   GTK_WIDGET
						(load_contact_photo (contact)));
				}
			}
			g_free (filename);
		}
	} else if (result == GTK_RESPONSE_DELETE_EVENT) {
		EContact *contact = get_current_contact ();
		
		if (contact && E_IS_CONTACT (contact)) {
			e_contact_set (contact, E_CONTACT_PHOTO, NULL);
			/* Re-display contact photo */
			gtk_button_set_image (GTK_BUTTON (widget), 
				     GTK_WIDGET (load_contact_photo (contact)));
		}
	}
	
	gtk_widget_destroy (filechooser);
}

typedef struct {
	EContact *contact;
	EContactField field_id;
} EContactChangeData;

static void
text_entry_changed (GtkEntry *entry, gpointer data)
{
	if (data) {
		EContactChangeData *d = (EContactChangeData *)data;
		gchar *string;
		string = (gchar *)gtk_entry_get_text (entry);
		if (g_utf8_strlen (string, -1) == 0)
			string = NULL;
		e_contact_set (d->contact, d->field_id, (gpointer)string);
	}
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
	g_signal_connect (G_OBJECT (entry), "destroy", 
			  G_CALLBACK (free_change_data), data);
	
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
	
	/* Display edit pane */
	/* ODD: Doing this after adding the widgets will cause the first view
	 * not to work... But only when using a viewport
	 */
	widget = glade_xml_get_widget (xml, "main_notebook");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 1);
	
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
	EContact *contact;

	/* Disable the edit/delete buttons and get the contact to edit */
	contact_selected_sensitive (FALSE);
	contact = get_current_contact ();
	
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
	EContact *contact;
	
	/* Commit changes */
	contact = get_current_contact ();
	if (contact)
		e_book_commit_contact(book, contact, NULL);

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
	xml = glade_xml_new (XML_FILE, "main_window", NULL);
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
						     "text", NAME, NULL);
	/* Create model and filter (for groups) */
	model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
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

	/* Select 'All' in the groups combobox */
	groups_combobox = GTK_COMBO_BOX (glade_xml_get_widget 
					 (xml, "groups_combobox"));
	gtk_combo_box_set_active (groups_combobox, 0);
	contacts_groups = NULL;

	/* Connect signal for selection changed event */
	selection = gtk_tree_view_get_selection (contacts_treeview);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (contact_selected), NULL);
	
	/* Load the system addressbook */
	contacts_table = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, 
						(GDestroyNotify)free_list_hash);
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
			  G_CALLBACK (contacts_added), NULL);
	g_signal_connect (G_OBJECT (book_view), "contacts_changed",
			  G_CALLBACK (contacts_changed), NULL);
	g_signal_connect (G_OBJECT (book_view), "contacts_removed",
			  G_CALLBACK (contacts_removed), NULL);
	/* TODO: Handle contacts_removed signal */
	/* Start */
	e_book_view_start (book_view);

	gtk_main ();

	return 0;
}
