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

typedef struct {
	const gchar *vcard_field;
	EContactField econtact_field; /* >0, gets used for pretty name */
	const gchar *pretty_name; /* Always takes priority over above */
	gboolean multi_line;
	guint priority;
} ContactsField;

#define REQUIRED 100	/* Contacts with priority < REQUIRED have to be shown */

/* List of always-available fields (ADR will have to be special-cased) */
/* TODO: Revise 'supported' fields */
static ContactsField contacts_fields[] = {
	{ "FN", E_CONTACT_FULL_NAME, NULL, FALSE, 10 },
	{ "TEL", 0, "Phone", FALSE, 20 },
	{ "EMAIL", 0, "Email", FALSE, 30 },
	{ "ADR", 0, "Address", FALSE, 40 },
	{ "NICKNAME", E_CONTACT_NICKNAME, NULL, FALSE, 110 },
	{ "URL", E_CONTACT_HOMEPAGE_URL, NULL, FALSE, 120 },
	{ "NOTE", E_CONTACT_NOTE, NULL, TRUE, 130 },
	{ NULL, 0, NULL, 0 }
};

enum {
	ADR_POBOX = 0,
	ADR_EXT,	/* Multiple line */
	ADR_STREET,	/* Multiple line */
	ADR_LOCAL,
	ADR_REGION,
	ADR_CODE,
	ADR_COUNTRY
};

static gchar *adr_fields[] = {
	"<span><b>P.O. Box:</b></span>",
	"<span><b>Extension:</b></span>",
	"<span><b>Street:</b></span>",
	"<span><b>Locality:</b></span>",
	"<span><b>Region:</b></span>",
	"<span><b>Post code:</b></span>",
	"<span><b>Country:</b></span>"
};

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

static const ContactsField *
contacts_get_contacts_field (const gchar *vcard_field)
{
	int i;
	
	for (i = 0; contacts_fields[i].vcard_field != NULL; i++) {
		if (strcmp (contacts_fields[i].vcard_field, vcard_field) == 0)
			return &contacts_fields[i];
	}
	
	return NULL;
}

static const gchar *
contacts_get_contacts_field_pretty_name (const ContactsField *field)
{
	if (field->pretty_name) {
		return field->pretty_name;
	} else if (field->econtact_field > 0) {
		return e_contact_pretty_name (field->econtact_field);
	} else
		return NULL;
}

void
change_photo (GtkWidget *button, EContact *contact)
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
						(load_contact_photo (contact)));
				}
			}
			g_free (filename);
		}
	} else if (result == NO_IMAGE) {
		if (contact && E_IS_CONTACT (contact)) {
			e_contact_set (contact, E_CONTACT_PHOTO, NULL);
			/* Re-display contact photo */
			gtk_button_set_image (GTK_BUTTON (button), 
				     GTK_WIDGET (load_contact_photo (contact)));
		}
	}
	
	gtk_widget_destroy (filechooser);
}

static void
contacts_widget_free_data (GtkEntry *entry, gpointer data)
{
	if (data)
		g_free (data);
}

static void
contacts_widget_free_list (GtkEntry *entry, gpointer data)
{
	if (data)
		g_list_free (data);
}

typedef struct {
	const gchar *name;
	gboolean selected;
} ContactsChoice;

static GList *
contacts_multiple_choice_chooser (const gchar *window_title,
				  const gchar *dialog_markup,
				  GList *choices,
				  gboolean allow_custom)
{
	GtkWidget *dialog, *label;
	
	dialog = glade_xml_get_widget (xml, "types_edit_dialog");
	gtk_window_set_title (GTK_WINDOW (dialog), window_title);
	
	label = glade_xml_get_widget (xml, "types_edit_label");
	gtk_label_set_markup (GTK_LABEL (label), dialog_markup);
	
	gtk_dialog_run (GTK_DIALOG (dialog));
	
	return NULL;
}

static void
contacts_entry_changed_cb (GtkEntry *entry, gpointer data)
{
	EContactChangeData *d = (EContactChangeData *)data;
	gchar *string;
	string = (gchar *)gtk_entry_get_text (entry);
		
	/* Remove old value and add new one */
	e_vcard_attribute_remove_values (d->attr);
	e_vcard_attribute_add_value (d->attr, string);
}

static void
contacts_buffer_changed_cb (GtkTextBuffer *buffer, gpointer data)
{
	EContactChangeData *d = (EContactChangeData *)data;
	gchar *string;
	GtkTextIter start, end;
	
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);
	
	string = (gchar *)gtk_text_buffer_get_text (buffer, &start, &end,
						    FALSE);
		
	/* Remove old value and add new one */
	e_vcard_attribute_remove_values (d->attr);
	e_vcard_attribute_add_value (d->attr, string);
}

static gboolean
contacts_edit_focus_in_cb (GtkWidget *widget, GdkEventFocus *event,
			   GtkContainer *button_align)
{
	GtkButton *button =
		GTK_BUTTON ((gtk_container_get_children (button_align))->data);
	gtk_button_set_relief (button, GTK_RELIEF_NORMAL);
	return FALSE;
}

static gboolean
contacts_edit_focus_out_cb (GtkWidget *widget, GdkEventFocus *event,
			   GtkContainer *button_align)
{
	GtkButton *button =
		GTK_BUTTON ((gtk_container_get_children (button_align))->data);
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	return FALSE;
}

/* Helper method derived from the method below to add focus in/out events
 * to the edit widgets.
 */
static void contacts_add_focus_events (GtkWidget *widget, GtkWidget *focus)
{
	if (GTK_IS_ENTRY (widget) || GTK_IS_TEXT_VIEW (widget)) {
		g_signal_connect (G_OBJECT (widget), "focus-in-event",
				  G_CALLBACK (contacts_edit_focus_in_cb),
				  focus);
		g_signal_connect (G_OBJECT (widget), "focus-out-event",
				  G_CALLBACK (contacts_edit_focus_out_cb),
				  focus);
	} else if (GTK_IS_CONTAINER (widget)) {
		GList *widgets =
			gtk_container_get_children (GTK_CONTAINER (widget));
		for (; widgets; widgets = widgets->next) {
			GtkWidget *child = GTK_WIDGET (widgets->data);
			contacts_add_focus_events (child, focus);
		}
	}
}

/* Helper method to get the string from a GtkLabel or GtkTextBuffer.
 * The method will recursively traverse containers until it finds either of
 * these widgets, or runs out of widgets.
 */
static gchar *contacts_get_text (GtkWidget *widget)
{
	if (GTK_IS_ENTRY (widget)) {
		return g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
	} else if (GTK_IS_TEXT_VIEW (widget)) {
		GtkTextIter start, end;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (
						GTK_TEXT_VIEW (widget));
		gtk_text_buffer_get_start_iter (buffer, &start);
		gtk_text_buffer_get_end_iter (buffer, &end);
		
		return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	} else if (GTK_IS_CONTAINER (widget)) {
		GList *widgets =
			gtk_container_get_children (GTK_CONTAINER (widget));
		for (; widgets; widgets = widgets->next) {
			GtkWidget *child = GTK_WIDGET (widgets->data);
			gchar *string = contacts_get_text (child);
			if (string)
				return string;
		}
	}
	return NULL;
}

static void
contacts_adr_changed_cb (GtkWidget *widget, GList *fields)
{
	gchar *pobox, *ext, *street, *local, *region, *code, *country;
	EVCardAttribute *attr = (EVCardAttribute *)fields->data;
	
	fields = fields->next;
	pobox = contacts_get_text (GTK_WIDGET (fields->data));
	fields = fields->next;
	ext = contacts_get_text (GTK_WIDGET (fields->data));
	fields = fields->next;
	street = contacts_get_text (GTK_WIDGET (fields->data));
	fields = fields->next;
	local = contacts_get_text (GTK_WIDGET (fields->data));
	fields = fields->next;
	region = contacts_get_text (GTK_WIDGET (fields->data));
	fields = fields->next;
	code = contacts_get_text (GTK_WIDGET (fields->data));
	fields = fields->next;
	country = contacts_get_text (GTK_WIDGET (fields->data));
	
	e_vcard_attribute_remove_values (attr);
	e_vcard_attribute_add_values (attr,
				      pobox ? pobox : "",
				      ext ? ext : "",
				      street ? street : "",
				      local ? local : "",
				      region ? region : "",
				      code ? code : "",
				      country ? country : "",
				      NULL);
	
	g_free (pobox); g_free (ext); g_free (street); g_free (local);
	g_free (region); g_free (code); g_free (country);
}

/* This function returns a widget derived from a particular EVCard attribute
 * providing a UI to view the attribute name and edit the type.
 * Returns GtkWidget * on success, NULL on failure
 */
static GtkWidget *
contacts_label_widget_new (EContact *contact, EVCardAttribute *attr,
			   const gchar *name)
{	
	GtkWidget *label, *button, *alignment;
	GList *types_list;
	gchar *type_string, *label_markup;

	/* Only support the ADR structured field */
	if (!e_vcard_attribute_is_single_valued (attr)) {
		if (strcmp (e_vcard_attribute_get_name (attr), "ADR") != 0)
			return NULL;
	}

	/* Create label/button text */
	types_list = contacts_get_string_list_from_types
			(e_vcard_attribute_get_params (attr));
	type_string = contacts_string_list_as_string (types_list, ", ");
	g_list_free (types_list);
	if (type_string) {
		label_markup = g_strdup_printf (
			"<span><b>%s:</b>\n<small>(%s)</small></span>",
			name, type_string);
		g_free (type_string);
	}
	else {
		label_markup = g_strdup_printf (
			"<span><b>%s:</b></span>", name);
	}

	/* Create widget */
	label = gtk_label_new (NULL);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_markup (GTK_LABEL (label), label_markup);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
	button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (button), label);
	gtk_widget_show (label);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_show (button);
	alignment = gtk_alignment_new (0.5, 0.0, 1.0, 0.0);
	gtk_container_add (GTK_CONTAINER (alignment), button);

	gtk_widget_set_name (alignment, e_vcard_attribute_get_name (attr));
	
	g_free (label_markup);
	
	return alignment;
}

/* This function returns a widget derived from a particular EVCard attribute
 * providing a UI to edit.
 * Returns GtkWidget * on success, NULL on failure
 */
static GtkWidget *
contacts_edit_widget_new (EContact *contact, EVCardAttribute *attr,
			  gboolean multi_line)
{	
	GtkWidget *edit_widget;
	const gchar *string;
	EContactChangeData *data;

	/* Create widget */
	/* Create data structure for changes */
	data = g_new (EContactChangeData, 1);
	data->contact = contact;
	data->attr = attr;

	if (!e_vcard_attribute_is_single_valued (attr)) {
		GList *values, *callback_data = NULL;
		GtkWidget *adr_table, *label, *entry;
		guint row;
		
		/* Support the ADR structured field */
		if (strcmp (e_vcard_attribute_get_name (attr), "ADR") != 0)
			return NULL;
		
		callback_data = g_list_append (NULL, attr);
		adr_table = gtk_table_new (1, 2, FALSE);
		gtk_table_set_col_spacings (GTK_TABLE (adr_table), 6);
		gtk_table_set_row_spacings (GTK_TABLE (adr_table), 6);
		values = e_vcard_attribute_get_values (attr);
		for (row = 0; values; values = values->next, row++) {
			label = gtk_label_new (NULL);
			gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
			gtk_label_set_markup (GTK_LABEL (label),
					      adr_fields[row]);
			gtk_label_set_justify (GTK_LABEL (label),
					       GTK_JUSTIFY_RIGHT);
			gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
			
			if (row == ADR_EXT || row == ADR_STREET) {
				GtkWidget *text_view;
				GtkTextBuffer *buffer;
				
				text_view = gtk_text_view_new ();
				buffer = gtk_text_view_get_buffer (
						GTK_TEXT_VIEW (text_view));
				gtk_text_buffer_set_text (buffer, values->data,
							  -1);
				gtk_text_view_set_editable (
					GTK_TEXT_VIEW (text_view), TRUE);
					
				gtk_widget_show (text_view);
				entry = gtk_scrolled_window_new (NULL, NULL);
				gtk_scrolled_window_set_policy (
					GTK_SCROLLED_WINDOW (entry),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
				gtk_scrolled_window_set_shadow_type (
					GTK_SCROLLED_WINDOW (entry),
					GTK_SHADOW_IN);
				gtk_container_add (GTK_CONTAINER (entry),
						   text_view);

				/* Connect signal for changes */
				g_signal_connect (G_OBJECT (buffer),
						  "changed", G_CALLBACK 
						  (contacts_adr_changed_cb),
						  callback_data);
			} else {
				entry = gtk_entry_new ();
				gtk_entry_set_text (GTK_ENTRY (entry),
						    values->data);
				/* Connect signal for changes */
				g_signal_connect (G_OBJECT (entry),
						  "changed", G_CALLBACK 
						  (contacts_adr_changed_cb),
						  callback_data);
			}
			
			gtk_table_attach (GTK_TABLE (adr_table), label, 0, 1,
					  row, row+1, GTK_FILL, GTK_FILL,
					  0, 0);
			gtk_table_attach (GTK_TABLE (adr_table), entry, 1, 2,
					  row, row+1, GTK_EXPAND | GTK_FILL, 0,
					  0, 0);
			gtk_widget_show (label);
			gtk_widget_show (entry);

			callback_data = g_list_append (callback_data, entry);
		}
		
		edit_widget = adr_table;

		/* Free address-widget data structure on destruction */
		g_signal_connect (G_OBJECT (edit_widget), "destroy", 
				  G_CALLBACK (contacts_widget_free_list),
				  callback_data);
	} else if (multi_line) {
		GtkTextView *view = GTK_TEXT_VIEW (gtk_text_view_new ());
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
		
		string = e_vcard_attribute_get_value (attr);
		gtk_text_buffer_set_text (buffer, string ? string : "", -1);
		gtk_text_view_set_editable (view, TRUE);
		
		edit_widget = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (
			GTK_SCROLLED_WINDOW (edit_widget),
			GTK_POLICY_AUTOMATIC,
			GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (
			GTK_SCROLLED_WINDOW (edit_widget), GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (edit_widget),
				   GTK_WIDGET (view));
		gtk_widget_show (GTK_WIDGET (view));
		
		/* Connect signal for changes */
		g_signal_connect (G_OBJECT (buffer), "changed", G_CALLBACK
				  (contacts_buffer_changed_cb), data);
	} else {
		string = e_vcard_attribute_get_value (attr);
		edit_widget = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (edit_widget),
				    string ? string : "");
		
		/* Connect signal for changes */
		g_signal_connect (G_OBJECT (edit_widget), "changed", G_CALLBACK
				  (contacts_entry_changed_cb), data);
	}
	gtk_widget_set_name (edit_widget, e_vcard_attribute_get_name (attr));

	/* Free change data structure on destruction */
	g_signal_connect (G_OBJECT (edit_widget), "destroy", 
			  G_CALLBACK (contacts_widget_free_data), data);
	
	return edit_widget;
}

static gint
contacts_widgets_list_sort_cb (GtkWidget *a, GtkWidget *b)
{
	const ContactsField *field1, *field2;
	
	field1 = contacts_get_contacts_field (gtk_widget_get_name (a));
	field2 = contacts_get_contacts_field (gtk_widget_get_name (b));
	
	return field1->priority - field2->priority;
}

static gint
contacts_widgets_list_find_cb (GtkWidget *a, guint *b)
{
	const ContactsField *field1;
	
	field1 = contacts_get_contacts_field (gtk_widget_get_name (a));
	
	return field1->priority - *b;
}

void
do_edit (EContact *contact)
{
	GtkWidget *widget;
	guint row, i;
	GList *attributes, *c, *d, *label_widgets, *edit_widgets;
	gchar *groups_string;
	
	/* Testing */
	attributes = e_vcard_get_attributes (&contact->parent);
	for (c = attributes; c; c = c->next) {
		EVCardAttribute *a = (EVCardAttribute*)c->data;
		GList *params = e_vcard_attribute_get_params (a);
		GList *values = e_vcard_attribute_get_values (a);
		g_printf ("%s:\n", e_vcard_attribute_get_name (a));
		for (; values; values = values->next) {
			g_printf ("  %s\n", (const gchar *)values->data);
		}
		if (params)
			g_print ("Attributes: \n");
		for (; params; params = params->next) {
			EVCardAttributeParam *p = (EVCardAttributeParam *)params->data;
			GList *paramvalues = e_vcard_attribute_param_get_values (p);
			g_printf ("    %s:\n", e_vcard_attribute_param_get_name (p));
			for (; paramvalues; paramvalues = paramvalues->next) {
				const gchar *value = (const gchar *)paramvalues->data;
				g_printf ("      o %s\n", value);
			}
		}
	}
	/* Testing */
	
	/* Switch to edit pane */
	/* ODD: Doing this after adding the widgets will cause the first view
	 * to appear empty... But only when using a viewport (Gtk bug?)
	 */
	widget = glade_xml_get_widget (xml, "main_notebook");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 1);
	
	/* Display contact photo */
	widget = glade_xml_get_widget (xml, "edit_photo_button");
	gtk_button_set_image (GTK_BUTTON (widget), 
			      GTK_WIDGET (load_contact_photo (contact)));
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (change_photo), contact);
	
	/* Display groups edit button */
	widget = glade_xml_get_widget (xml, "groups_button");
	c = e_contact_get (contact, E_CONTACT_CATEGORY_LIST);
	if (c) {
		groups_string = contacts_string_list_as_string (c, ",\n");
		gtk_button_set_label (GTK_BUTTON (widget), groups_string);
		g_free (groups_string);
		gtk_widget_show (widget);
	} else {
		gtk_button_set_label (GTK_BUTTON (widget), "Add group");
	}
	
	/* Create edit pane widgets */
	attributes = e_vcard_get_attributes (&contact->parent);
	label_widgets = NULL;
	edit_widgets = NULL;
	for (c = attributes; c; c = c->next) {
		EVCardAttribute *a = (EVCardAttribute*)c->data;
		const gchar *name = e_vcard_attribute_get_name (a);
		const ContactsField *field = contacts_get_contacts_field (name);
		
		if (field) {
			GtkWidget *label, *edit;
			const gchar *pretty_name = 
				contacts_get_contacts_field_pretty_name (field);
			
			label = contacts_label_widget_new (contact, a,
							   pretty_name);
			edit = contacts_edit_widget_new (contact, a,
							 field->multi_line);
			
			if (label && edit) {
				label_widgets = g_list_append (label_widgets,
							       label);
				edit_widgets = g_list_append (edit_widgets,
							      edit);
			}
		}
	}
	
	/* Add any missing required widgets */
	for (i = 0; contacts_fields[i].vcard_field != NULL; i++) {
		if (contacts_fields[i].priority >= REQUIRED)
			continue;
		if (!g_list_find_custom (label_widgets,
					&contacts_fields[i].priority,
					(GCompareFunc)
					 contacts_widgets_list_find_cb)) {
			const gchar *pretty_name =
				contacts_get_contacts_field_pretty_name
				    (contacts_get_contacts_field
					(contacts_fields[i].vcard_field));
			EVCardAttribute *attr = e_vcard_attribute_new (
					"", contacts_fields[i].vcard_field);
					
			/* Special-casing the ADR field */
			if (strcmp (contacts_fields[i].vcard_field, "ADR") == 0)
				e_vcard_add_attribute_with_values (
					&contact->parent, attr, "", "", "", "",
					"", "", "", NULL);
			else
				e_vcard_add_attribute_with_value (
					&contact->parent, attr, "");
			GtkWidget *label, *edit;

			label = contacts_label_widget_new (contact, attr,
							   pretty_name);
			edit = contacts_edit_widget_new (contact, attr,
						contacts_fields[i].multi_line);
			
			if (label && edit) {
				label_widgets = g_list_append (label_widgets,
							       label);
				edit_widgets = g_list_append (edit_widgets,
							      edit);
			}
		}
	}
	
	/* Sort widgets into order, display and sort out focus -
	 * The first entry has focus and the order of focus is changed so that
	 * each entry focuses after the next.
	 */
	widget = glade_xml_get_widget (xml, "edit_table");
	g_list_sort (label_widgets,
		     (GCompareFunc)contacts_widgets_list_sort_cb);
	g_list_sort (edit_widgets, (GCompareFunc)contacts_widgets_list_sort_cb);
	for (c = label_widgets, d = edit_widgets, row = 0; (c) && (d);
	     c = c->next, d = d->next, row++) {
		contacts_add_focus_events (GTK_WIDGET (d->data),
					   GTK_WIDGET (c->data));
		gtk_table_attach (GTK_TABLE (widget), GTK_WIDGET (c->data),
				  0, 1, row, row+1,
				  GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach (GTK_TABLE (widget), GTK_WIDGET (d->data),
				  1, 2, row, row+1,
				  GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
		gtk_widget_show (c->data);
		gtk_widget_show (d->data);
	}
	c = g_list_concat (label_widgets, edit_widgets);
	gtk_container_set_focus_chain (GTK_CONTAINER (widget), c);
	
	widget = glade_xml_get_widget (xml, "main_window");
	gtk_window_set_focus (GTK_WINDOW (widget),
			      GTK_WIDGET (edit_widgets->data));
	g_list_free (c);

	widget = glade_xml_get_widget (xml, "main_window");
	gtk_window_set_title (GTK_WINDOW (widget), "Edit contact");
	
	/* Connect close button */
	widget = glade_xml_get_widget (xml, "edit_done_button");
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (edit_done), contact);
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
	/* Start */
	e_book_view_start (book_view);

	gtk_main ();

	return 0;
}
