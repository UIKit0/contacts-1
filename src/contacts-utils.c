/* 
 *  Contacts - A small libebook-based address book.
 *
 *  Authored By Chris Lord <chris@linux.intel.com>
 *
 *  Copyright (c) 2005, 2009 Intel Corporation
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

#include <config.h>

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>

#include "contacts-utils.h"
#include "contacts-defs.h"
#include "contacts-main.h"

/******************************************************************************/

/* List of always-available fields */
/* TODO: Revise 'supported' fields */
/* Note: PHOTO and CATEGORIES are special-cased (see contacts_edit_pane_show) */
static const ContactsField contacts_fields[] = {
	{ "FN", E_CONTACT_FULL_NAME, NULL, FALSE, 10, TRUE },
	{ "ORG", E_CONTACT_ORG, NULL, FALSE, 15, TRUE },
	{ "TEL", 0, N_("Phone"), FALSE, 20, FALSE },
	{ "EMAIL", 0, N_("Email"), FALSE, 30, FALSE },
	{ "X-JABBER", 0, N_("Jabber"), FALSE, 35, FALSE },
	{ "X-MSN", 0, N_("MSN"), FALSE, 35, FALSE },
	{ "X-YAHOO", 0, N_("Yahoo"), FALSE, 35, FALSE },
	{ "X-AIM", 0, N_("AIM"), FALSE, 35, FALSE },
	{ "X-ICQ", 0, N_("ICQ"), FALSE, 35, FALSE },
	{ "X-GROUPWISE", 0, N_("Groupwise"), FALSE, 35, FALSE },
	{ "ADR", 0, N_("Address"), FALSE, 40, FALSE },
	{ "NICKNAME", E_CONTACT_NICKNAME, NULL, FALSE, 110, TRUE },
	{ "BDAY", E_CONTACT_BIRTH_DATE, NULL, FALSE, 120, TRUE },
	{ "URL", E_CONTACT_HOMEPAGE_URL, N_("Homepage"), FALSE, 130, FALSE },
	{ "NOTE", E_CONTACT_NOTE, NULL, TRUE, 140, TRUE },
	{ NULL }
};



static const ContactsStructuredField contacts_sfields[] = {
	{ "ADR", 0, N_("PO Box"), FALSE, -1 },
	{ "ADR", 1, N_("Ext."), TRUE, 0 },
	{ "ADR", 2, N_("Street"), TRUE, 1 },
	{ "ADR", 3, N_("City"), FALSE, 1 },
	{ "ADR", 4, N_("Provice"), FALSE, 1 },
	{ "ADR", 5, N_("Postal Code"), FALSE, 1 },
	{ "ADR", 6, N_("Country"), FALSE, 1 },
	{ NULL, 0, NULL, FALSE }
};

/* Translators: Home, Cell, Office, Fax and Other are types of telephone attributes */
static const TypeTuple tel_types[] =
{
	{"HOME", N_("Home")},
	{"CELL", N_("Cell")},
	{"WORK", N_("Work")},
	{"FAX", N_("Fax")},
	{"OTHER", N_("Other")},
	{NULL}
};

/* Translators: Home, Office and Other are types of contact attributes */
static const TypeTuple generic_types[] =
{
	{"HOME", N_("Home")},
	{"WORK", N_("Work")},
	{"OTHER", N_("Other")},
	{NULL}
};

const TypeTuple *
contacts_get_field_types (const gchar *attr_name)
{
	if (!attr_name)
		return NULL;

	if (!strcmp (attr_name, "TEL"))
		return tel_types;
	else if (!strcmp (attr_name, "EMAIL") || !strcmp (attr_name, "ADR"))
		return generic_types;
	else
		return NULL;
}

const ContactsStructuredField *
contacts_get_structured_field (const gchar *attr_name, guint field)
{
	guint i;
	
	for (i = 0; contacts_sfields[i].attr_name; i++) {
		if (strcmp (contacts_sfields[i].attr_name, attr_name) == 0) {
			if (contacts_sfields[i].field == field)
				return &contacts_sfields[i];
		}
	}
	
	return NULL;
}

guint
contacts_get_structured_field_size (const gchar *attr_name)
{
	guint i, size = 1;
	
	for (i = 0; contacts_sfields[i].attr_name; i++)
		if (strcmp (contacts_sfields[i].attr_name, attr_name) == 0)
			if (contacts_sfields[i].field+1 > size)
				size = contacts_sfields[i].field+1;
	
	return size;
}

const ContactsField *
contacts_get_contacts_field (const gchar *vcard_field)
{
	guint i;
	
	for (i = 0; (contacts_fields[i].vcard_field) && (vcard_field); i++) {
		if (strcmp (contacts_fields[i].vcard_field, vcard_field) == 0)
			return &contacts_fields[i];
	}
	
	return NULL;
}

const ContactsField *
contacts_get_contacts_fields ()
{
	return contacts_fields;
}

const gchar *
contacts_field_pretty_name (const ContactsField *field)
{
	if (field->pretty_name) {
		return gettext(field->pretty_name);
	} else if (field->econtact_field > 0) {
		return e_contact_pretty_name (field->econtact_field);
	} else
		return NULL;
}

gint
contacts_compare_attributes (EVCardAttribute *attrA, EVCardAttribute *attrB)
{
	gint a = 0, b = 0, i;
	const gchar *nameA, *nameB;

	nameA = e_vcard_attribute_get_name (attrA);
	nameB = e_vcard_attribute_get_name (attrB);

	for (i = 0; contacts_fields[i].vcard_field; i++)
	{
		if (a == 0 && strcmp (nameA, contacts_fields[i].vcard_field) == 0)
			a = contacts_fields[i].priority;
		if (b == 0 && strcmp (nameB, contacts_fields[i].vcard_field) == 0)
			b = contacts_fields[i].priority;
	}

	return a - b;
}

EContact *
contacts_contact_from_tree_path (GtkTreeModel *model, GtkTreePath *path, GHashTable *contacts_table)
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter (model, &iter, path)) {
		gchar *uid;
		EContactListHash *hash;
		gtk_tree_model_get (model, &iter, CONTACT_UID_COL, &uid, -1);
		if (uid) {
			hash = g_hash_table_lookup (contacts_table, uid);
			g_free (uid);
			if (hash)
				return hash->contact;
		}
	}
	return NULL;
}

EContact *
contacts_contact_from_selection (GtkTreeSelection *selection,
				 GHashTable *contacts_table)
{
	GtkTreeModel *model;
	GList *selected_paths;
	GtkTreePath *path;
	EContact *result;
	
	if (!selection || !GTK_IS_TREE_SELECTION (selection))
		return NULL;

	selected_paths = gtk_tree_selection_get_selected_rows (selection, &model);
	if (!selected_paths)
		return NULL;
	path = selected_paths->data;

	result = contacts_contact_from_tree_path (model, path, contacts_table);

	g_list_foreach (selected_paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (selected_paths);
	return result;
}

EContact *
contacts_get_selected_contact (ContactsData *data, GHashTable *contacts_table)
{
	GtkWidget *widget;
	GtkTreeSelection *selection;
	EContact *contact;

	/* Get the currently selected contact */
	widget = data->ui->contacts_treeview;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	
	if (!selection || !GTK_IS_TREE_SELECTION (selection))
		return NULL;
		
	contact = contacts_contact_from_selection (selection, contacts_table);
	
	return contact;
}

void
contacts_set_selected_contact (ContactsData *data, const gchar *uid)
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeIter iter;

	treeview = GTK_TREE_VIEW (
		data->ui->contacts_treeview);
	model = /*gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (*/
		gtk_tree_view_get_model (treeview)/*))*/;
	
	if (!gtk_tree_model_get_iter_first (model, &iter)) return;
	
	do {
		const gchar *ruid;
		gtk_tree_model_get (model, &iter, CONTACT_UID_COL, &ruid, -1);
		if (strcmp (uid, ruid) == 0) {
			GtkTreeSelection *selection =
				gtk_tree_view_get_selection (treeview);
			gtk_tree_selection_unselect_all (selection);
			if (selection)
				gtk_tree_selection_select_iter (
					selection, &iter);
			return;
		}
	} while (gtk_tree_model_iter_next (model, &iter));

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

GdkPixbuf *
contacts_load_photo (EContact *contact)
{
	GdkPixbuf *pixbuf = NULL;
	EContactPhoto *photo;
	GdkPixbufLoader *loader;
	gint iconwidth, iconheight;
	GError *error = NULL;
	gchar *filename = NULL;

	/* Retrieve contact picture and resize */
	photo = e_contact_get (contact, E_CONTACT_PHOTO);

	if (photo) {

		loader = gdk_pixbuf_loader_new ();
		if (loader) {
			g_signal_connect (G_OBJECT (loader),
					  "size-prepared",
					  G_CALLBACK (contact_photo_size),
					  NULL);

#if HAVE_PHOTO_TYPE
			switch (photo->type) {
			case E_CONTACT_PHOTO_TYPE_INLINED :
				gdk_pixbuf_loader_write (loader,
					photo->data.inlined.data,
					photo->data.inlined.length, NULL);
				break;
			case E_CONTACT_PHOTO_TYPE_URI :
			default :
				gdk_pixbuf_loader_close (loader, NULL);
				g_object_unref (loader);
				loader = NULL;

				gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, 
					&iconwidth, &iconheight);

				/* Assume local */
				filename = g_filename_from_uri (photo->data.uri, 
						NULL, 
						NULL);

				if (!filename)
					break;

				pixbuf = gdk_pixbuf_new_from_file_at_size (
					filename,
					iconwidth, iconheight,
					&error);

				if (!pixbuf)
				{
				  g_warning (G_STRLOC ": Error loading pixbuf: %s",
						  error->message);
				}
				break;
			}
#else
			gdk_pixbuf_loader_write (loader, (const guchar *)
				photo->data, photo->length, NULL);
#endif
			if (loader) {
				gdk_pixbuf_loader_close (loader, NULL);
				pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

				/* Must ref here because we close the loader */
				g_object_ref (pixbuf);
				g_object_unref (loader);
			}
		}
		e_contact_photo_free (photo);
	}
	return pixbuf;
}

/* This removes any vCard attributes that are just "", or have no associated
 * value. Evolution tends to add empty fields like this a lot - as does
 * contacts, to show required fields (but it removes them after editing, via
 * this function).
 */
void
contacts_clean_contact (EContact *contact)
{
	GList *attributes, *c, *trash = NULL;

	attributes = e_vcard_get_attributes (E_VCARD (contact));

	for (c = attributes; c; c = c->next) {
		EVCardAttribute *a = (EVCardAttribute*)c->data;
		GList *values = e_vcard_attribute_get_values (a);
		gboolean remove = TRUE;
		for (; values; values = values->next) {
			if (g_utf8_strlen ((const gchar *)values->data, -1) > 0)
				remove = FALSE;
		}
		if (remove) {
			trash = g_list_prepend (trash, a);
		}
	}

	for (c = g_list_first (trash); c; c = c->next)
		e_vcard_remove_attribute (E_VCARD (contact), c->data);

	g_list_free (trash);
}

gboolean
contacts_contact_is_empty (EContact *contact)
{
	GList *attributes, *c;
	
	attributes = e_vcard_get_attributes (E_VCARD (contact));
	for (c = attributes; c; c = c->next) {
		EVCardAttribute *a = (EVCardAttribute*)c->data;
		GList *values = e_vcard_attribute_get_values (a);
		for (; values; values = values->next) {
			if (g_utf8_strlen ((const gchar *)values->data, -1) > 0)
				return FALSE;
		}
	}
	
	return TRUE;
}

gchar *
contacts_string_list_as_string (GList *list, const gchar *separator,
				gboolean include_empty)
{
	gchar *old_string, *new_string = NULL;
	GList *c;
	
	if (!include_empty)
		for (; ((list) && (strlen ((const gchar *)list->data) == 0));
		     list = list->next);

	if (!list) return NULL;
	
	new_string = g_strdup (list->data);
	for (c = list->next; c; c = c->next) {
		if ((strlen ((const gchar *)c->data) > 0) || (include_empty)) {
			old_string = new_string;
			new_string = g_strdup_printf ("%s%s%s", new_string,
				separator, (const gchar *)c->data);
			g_free (old_string);
		}
	}
	
	return new_string;
}

GList *
contacts_get_types (GList *params)
{
	GList *list = NULL;

	for (; params; params = params->next) {
		EVCardAttributeParam *p = (EVCardAttributeParam *)params->data;
		if (strcmp ("TYPE", e_vcard_attribute_param_get_name (p)) == 0)
			list = g_list_append (list, p);
	}
	
	return list;
}

GList *
contacts_get_type_strings (GList *params)
{
	GList *list = NULL;

	for (; params; params = params->next) {
		EVCardAttributeParam *p = (EVCardAttributeParam *)params->data;
		if (strcmp ("TYPE", e_vcard_attribute_param_get_name (p)) == 0){
			GList *types = e_vcard_attribute_param_get_values (p);
			for (; types; types = types->next) {
				list = g_list_append (list, types->data);
			}
		}
	}
	
	return list;
}

void
contacts_choose_photo (GtkWidget *button, EContact *contact)
{
	GtkWidget *filechooser, *image;
	GdkPixbuf *pixbuf;
	GtkFileFilter *filter;
	gint result;
	GList *widgets;
	
	/* Get a filename */
	/* Note: I don't use the GTK_WINDOW cast as gtk_widget_get_ancestor
	 * can return NULL and this would probably throw a critical Gtk error.
	 */
	filechooser = gtk_file_chooser_dialog_new (_("Open image"),
						   (GtkWindow *)
						   gtk_widget_get_ancestor (
						   	button,
						   	GTK_TYPE_WINDOW),
						   GTK_FILE_CHOOSER_ACTION_OPEN,
						   GTK_STOCK_CANCEL,
						   GTK_RESPONSE_CANCEL,
						   GTK_STOCK_OPEN,
						   GTK_RESPONSE_ACCEPT,
						   _("No image"),
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
	widgets = contacts_set_widgets_desensitive (
		gtk_widget_get_ancestor (button, GTK_TYPE_WINDOW));
	result = gtk_dialog_run (GTK_DIALOG (filechooser));
	if (result == GTK_RESPONSE_ACCEPT) {
		gchar *filename = gtk_file_chooser_get_filename 
					(GTK_FILE_CHOOSER (filechooser));
		if (filename) {
			if (contact) {
				EContactPhoto new_photo;
				guchar **data;
				gsize *length;
#if HAVE_PHOTO_TYPE
				new_photo.type = E_CONTACT_PHOTO_TYPE_INLINED;
				data = &new_photo.data.inlined.data;
				length = &new_photo.data.inlined.length;
				new_photo.data.inlined.mime_type = NULL;
#else
				data = &new_photo.data;
				length = &new_photo.length;
#endif
				if (g_file_get_contents (filename, 
							 (gchar **)data,
							 (gsize *)length,
							 NULL)) {
					e_contact_set (contact, E_CONTACT_PHOTO,
						       &new_photo);
					g_free (*data);
					pixbuf = contacts_load_photo (contact);
					image = gtk_image_new_from_pixbuf (pixbuf);
					gtk_button_set_image (GTK_BUTTON (button),
							      image);
					g_object_unref (pixbuf);
				}
			}
			g_free (filename);
		}
	} else if (result == NO_IMAGE) {
		if (contact && E_IS_CONTACT (contact)) {
			e_contact_set (contact, E_CONTACT_PHOTO, NULL);
			image = gtk_image_new_from_icon_name ("stock_person", 
							      GTK_ICON_SIZE_DIALOG);
			gtk_button_set_image (GTK_BUTTON (button), image);
		}
	}
	
	contacts_set_widgets_sensitive (widgets);
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
			  (GTK_TREE_VIEW (hash->contacts_data->ui->contacts_treeview)))));
		gtk_list_store_remove (model, &hash->iter);
		g_object_unref (hash->contact);
		g_free (hash);
	}
}

/* Helper method to harvest user-input from GtkContainer's */
GList *
contacts_entries_get_values (GtkWidget *widget, GList *list) {
	if (GTK_IS_ENTRY (widget)) {
		return g_list_append (list, g_strdup (
				gtk_entry_get_text (GTK_ENTRY (widget))));
	} else if (GTK_IS_COMBO_BOX_ENTRY (widget)) {
		return g_list_append (list, g_strdup (
			gtk_entry_get_text (
				GTK_ENTRY (GTK_BIN (widget)->child))));
	} else if (GTK_IS_TEXT_VIEW (widget)) {
		GtkTextIter start, end;
		GtkTextBuffer *buffer =
			gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
		gtk_text_buffer_get_start_iter (buffer, &start);
		gtk_text_buffer_get_end_iter (buffer, &end);
		return g_list_append (list, 
			gtk_text_buffer_get_text (buffer, &start, &end, FALSE));
	} else if (GTK_IS_CONTAINER (widget)) {
		GList *c, *children =
			gtk_container_get_children (GTK_CONTAINER (widget));
		for (c = children; c; c = c->next) {
			list = contacts_entries_get_values (
				GTK_WIDGET (c->data), list);
		}
		g_list_free (children);
	}
	
	return list;
}

static void
contacts_chooser_treeview_row_activated (GtkTreeView *tree, gpointer data)
{
	GtkWidget *dialog = g_object_get_data (G_OBJECT (tree), "parent_dialog");
	gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
}

gboolean
contacts_chooser (ContactsData *data, const gchar *title, const gchar *label_markup,
		  GList *choices, GList *chosen, gboolean allow_custom,gboolean multiple_choice,
		  GList **results)
{
	GList *c, *d, *widgets;
	GtkWidget *label_widget;
	GtkWidget *dialog = data->ui->chooser_dialog;
	GtkTreeView *tree = GTK_TREE_VIEW (data->ui->chooser_treeview);
	GtkTreeModel *model = gtk_tree_view_get_model (tree);
	GtkWidget *add_custom = data->ui->chooser_add_hbox;
	gint dialog_code;
	gboolean returnval = FALSE;
	
	if (allow_custom)
		gtk_widget_show (add_custom);
	else
		gtk_widget_hide (add_custom);
	
	label_widget = data->ui->chooser_label;
	if (label_markup) {
		gtk_label_set_markup (GTK_LABEL (label_widget), label_markup);
		gtk_widget_show (label_widget);
	} else {
		gtk_widget_hide (label_widget);
	}
	
	gtk_list_store_clear (GTK_LIST_STORE (model));
	for (c = choices, d = chosen; c; c = c->next) {
		GtkTreeIter iter;
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    CHOOSER_NAME_COL,
				    (const gchar *)c->data, -1);
		if (multiple_choice) {
			if (d) {
				gboolean chosen =
					(gboolean)GPOINTER_TO_INT (d->data);
				gtk_list_store_set (GTK_LIST_STORE (model),
						    &iter, CHOOSER_TICK_COL,
						    chosen, -1);
				d = d->next;
			} else {
				gtk_list_store_set (GTK_LIST_STORE (model),
						    &iter, CHOOSER_TICK_COL,
						    FALSE, -1);
			}
		}
	}
	if (multiple_choice)
		gtk_tree_view_column_set_visible (gtk_tree_view_get_column (
			tree, CHOOSER_TICK_COL), TRUE);
	else
		gtk_tree_view_column_set_visible (gtk_tree_view_get_column (
			tree, CHOOSER_TICK_COL), FALSE);
	
	gtk_window_set_title (GTK_WINDOW (dialog), title);
	
	widgets = contacts_set_widgets_desensitive (
		data->ui->main_window);

	/*
	 * Set the parent dialog as data on the treeview.
	 * Passing the dialog through the user data callback causes problems
	 * because of the interaction between the application and the dialog
	 * main loops.
	 */
	g_object_set_data (G_OBJECT (tree), "parent_dialog", dialog);

	if (!multiple_choice)
		g_signal_connect (G_OBJECT(tree), "row-activated", (GCallback) contacts_chooser_treeview_row_activated, NULL);

	dialog_code = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);
	if (dialog_code == GTK_RESPONSE_OK) {
		if (multiple_choice) {
			GtkTreeIter iter;
			gboolean valid =
				gtk_tree_model_get_iter_first (model, &iter);
			
			while (valid) {
				gboolean selected;
				gtk_tree_model_get (model, &iter,
					CHOOSER_TICK_COL, &selected, -1);
				if (selected) {
					gchar *name;
					gtk_tree_model_get (model, &iter,
						CHOOSER_NAME_COL, &name, -1);
					*results =
						g_list_append (*results, name);
				}
			
				valid = gtk_tree_model_iter_next (model, &iter);
			}
			
			returnval = TRUE;
		} else {
			gchar *selection_name;
			GtkTreeSelection *selection =
				gtk_tree_view_get_selection (tree);
			GtkTreeIter iter;
			
			if (!gtk_tree_selection_get_selected (
				selection, NULL, &iter)) {
				returnval = FALSE;
			} else {
				gtk_tree_model_get (model, &iter,
					CHOOSER_NAME_COL, &selection_name, -1);
				*results = g_list_append (NULL, selection_name);
				returnval = TRUE;
			}
		}
	}
	
	contacts_set_widgets_sensitive (widgets);
	g_list_free (widgets);

	return returnval;
}

static GList *
contacts_set_widget_desensitive_recurse (GtkWidget *widget, GList **widgets)
{
	if (GTK_IS_WIDGET (widget)) {
		gboolean sensitive;
		
		g_object_get (G_OBJECT (widget), "sensitive", &sensitive, NULL);
		if (sensitive) {
			gtk_widget_set_sensitive (widget, FALSE);
			*widgets = g_list_append (*widgets, widget);
		}
		
		if (GTK_IS_TABLE (widget) || GTK_IS_HBOX (widget) ||
		    GTK_IS_VBOX (widget) || GTK_IS_EVENT_BOX (widget)) {
			GList *c, *children = gtk_container_get_children (
				GTK_CONTAINER (widget));
			
			for (c = children; c; c = c->next) {
				contacts_set_widget_desensitive_recurse (
					c->data, widgets);
			}
			g_list_free (children);
		}
	}
	
	return *widgets;
}

GList *
contacts_set_widgets_desensitive (GtkWidget *widget)
{
	GList *list = NULL;
	
	contacts_set_widget_desensitive_recurse (widget, &list);
	
	return list;
}

void
contacts_set_widgets_sensitive (GList *widgets)
{
	GList *w;
	
	for (w = widgets; w; w = w->next) {
		gtk_widget_set_sensitive (GTK_WIDGET (w->data), TRUE);
	}
}

gboolean
contacts_window_get_is_visible (GtkWindow *window)
{
	GdkWindowState  state;
	GdkWindow      *gdk_window;

	g_return_val_if_fail (GTK_IS_WINDOW (window), FALSE);

	gdk_window = GTK_WIDGET (window)->window;
	if (!gdk_window) {
		return FALSE;
	}

	state = gdk_window_get_state (gdk_window);
	if (state & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED)) {
		return FALSE;
	}

	return TRUE;
}


/* Takes care of moving the window to the current workspace. */
void
contacts_window_present (ContactsData *data)
{
	guint32 timestamp;
	GtkWidget *window = gtk_widget_get_toplevel (data->ui->summary_vbox);

	g_return_if_fail (GTK_WIDGET_TOPLEVEL (window));
	g_return_if_fail (GTK_IS_WINDOW (window));

	/* There are three cases: hidden, visible, visible on another
	 * workspace.
	 */
	if (!contacts_window_get_is_visible (GTK_WINDOW (window))) {
		/* Hide it so present brings it to the current workspace. */
		gtk_widget_hide (GTK_WIDGET (window));
	}

	timestamp = gtk_get_current_event_time ();
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), FALSE);
	gtk_window_present_with_time (GTK_WINDOW (window), timestamp);
}

