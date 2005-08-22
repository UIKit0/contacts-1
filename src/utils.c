
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libebook/e-book.h>

#include "utils.h"
#include "defs.h"

/* The following functions taken from
 * http://svn.o-hand.com/repos/kozo/server/src/kozo-utf8.c
 */
 /*****************************************************************************/
G_INLINE_FUNC char *
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

const char *
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

gunichar *
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

EContact *
contacts_contact_from_selection (GtkTreeSelection *selection,
				 GHashTable *contacts_table)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	if (!selection || !GTK_IS_TREE_SELECTION (selection))
		return NULL;
		
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		const gchar *uid;
		EContactListHash *hash;
		gtk_tree_model_get (model, &iter, CONTACT_UID_COL, &uid, -1);
		if (uid) {
			hash = g_hash_table_lookup (contacts_table, uid);
			if (hash)
				return hash->contact;
		}
	}
	return NULL;
}

EContact *
contacts_get_selected_contact (GladeXML *xml, GHashTable *contacts_table)
{
	GtkWidget *widget;
	GtkTreeSelection *selection;
	EContact *contact;

	/* Get the currently selected contact */
	widget = glade_xml_get_widget (xml, "contacts_treeview");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	
	if (!selection || !GTK_IS_TREE_SELECTION (selection))
		return NULL;
		
	contact = contacts_contact_from_selection (selection, contacts_table);
	
	return contact;
}

/* Helper method to set edit/delete sensitive/insensitive */
void
contact_selected_sensitive (GladeXML *xml, gboolean sensitive)
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

GtkImage *
contacts_load_photo (EContact *contact)
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

/* This removes any vCard attributes that are just "", or have no associated
 * value. Evolution tends to add empty fields like this a lot, as does
 * contacts to show required fields.
 * TODO: This really doesn't need to be recursive.
 */
void
contacts_clean_contact (EContact *contact)
{
	GList *attributes, *c;

	attributes = e_vcard_get_attributes (&contact->parent);
	for (c = attributes; c; c = c->next) {
		EVCardAttribute *a = (EVCardAttribute*)c->data;
		GList *values = e_vcard_attribute_get_values (a);
		gboolean remove = TRUE;
		for (; values; values = values->next) {
			if (g_utf8_strlen ((const gchar *)values->data, -1) > 0)
				remove = FALSE;
		}
		if (remove) {
			e_vcard_remove_attribute (&contact->parent, a);
			contacts_clean_contact (contact);
			break;
		}
	}
}

gchar *
contacts_string_list_as_string (GList *list, const gchar *separator)
{
	gchar *old_string, *new_string;
	GList *c;
	
	if (!list) return NULL;
	
	new_string = g_strdup (list->data);
	for (c = list->next; c; c = c->next) {
		old_string = new_string;
		new_string = g_strdup_printf ("%s%s%s", new_string, separator,
					      (const gchar *)c->data);
		g_free (old_string);
	}
	
	return new_string;
}

GList *
contacts_get_string_list_from_types (GList *params)
{
	GList *list = NULL;

	for (; params; params = params->next) {
		EVCardAttributeParam *p = (EVCardAttributeParam *)params->data;
		GList *pvs = e_vcard_attribute_param_get_values (p);
		if (strcmp ("TYPE", e_vcard_attribute_param_get_name (p)) != 0)
			continue;
		for (; pvs; pvs = pvs->next)
			list = g_list_append (list, pvs->data);
	}
	
	return list;
}

void
contacts_choose_photo (GtkWidget *button, EContact *contact)
{
	GtkWidget *widget;
	GtkWidget *filechooser;
	GtkFileFilter *filter;
	gint result;
	GladeXML *xml = glade_get_widget_tree (button);
	
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
						   NO_IMAGE,
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
	if (result == GTK_RESPONSE_ACCEPT) {
		gchar *filename = gtk_file_chooser_get_filename 
					(GTK_FILE_CHOOSER (filechooser));
		if (filename) {
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
					   GTK_BUTTON (button), 
					   GTK_WIDGET
						(contacts_load_photo (contact)));
				}
			}
			g_free (filename);
		}
	} else if (result == NO_IMAGE) {
		if (contact && E_IS_CONTACT (contact)) {
			e_contact_set (contact, E_CONTACT_PHOTO, NULL);
			/* Re-display contact photo */
			gtk_button_set_image (GTK_BUTTON (button), 
				     GTK_WIDGET (contacts_load_photo (contact)));
		}
	}
	
	gtk_widget_destroy (filechooser);
}

void
contacts_free_list_hash (gpointer data)
{
	EContactListHash *hash = (EContactListHash *)data;
	
	if (hash) {
		GtkListStore *model = GTK_LIST_STORE 
			(gtk_tree_model_filter_get_model 
			 (GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model 
			  (GTK_TREE_VIEW (glade_xml_get_widget
					(hash->xml, "contacts_treeview"))))));
		gtk_list_store_remove (model, &hash->iter);
		g_object_unref (hash->contact);
		g_free (hash);
	}
}
