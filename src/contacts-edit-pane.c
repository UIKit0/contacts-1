
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


/* List of always-available fields */
/* TODO: Revise 'supported' fields */
/* Note: PHOTO and CATEGORIES are special-cased (see contacts_edit_pane_show) */
static ContactsField contacts_fields[] = {
	{ "FN", E_CONTACT_FULL_NAME, NULL, FALSE, 10 },
	{ "TEL", 0, "Phone", FALSE, 20 },
	{ "EMAIL", 0, "Email", FALSE, 30 },
	{ "ADR", 0, "Address", FALSE, 40 },
	{ "NICKNAME", E_CONTACT_NICKNAME, NULL, FALSE, 110 },
	{ "URL", E_CONTACT_HOMEPAGE_URL, "Homepage", FALSE, 120 },
	{ "NOTE", E_CONTACT_NOTE, NULL, TRUE, 130 },
	{ NULL, 0, NULL, FALSE, 0 }
};

/* Contains structured field data
 * TODO: Replace this with a less ugly construct?
 */

typedef struct {
	const gchar *attr_name;
	guint field;
	const gchar *subfield_name;
	gboolean multiline;
} ContactsStructuredField;

static ContactsStructuredField contacts_sfields[] = {
	{ "ADR", 0, "PO Box", FALSE },
	{ "ADR", 1, "Ext.", TRUE },
	{ "ADR", 2, "Street", TRUE },
	{ "ADR", 3, "Locality", FALSE },
	{ "ADR", 4, "Region", FALSE },
	{ "ADR", 5, "Post Code", FALSE },
	{ "ADR", 6, "Country", FALSE },
	{ NULL, 0, NULL, FALSE }
};

/* TODO: Would adding a struct for this be gratuititous? */
static gchar **contacts_field_types[] = {
	(gchar *[]){ "TEL", "Home", "Msg", "Work", "Pref", "Voice", "Fax",
			    "Cell", "Video", "Pager", "BBS", "Modem", "Car",
			    "ISDN", "PCS", NULL },
	(gchar *[]){ "EMAIL", "Internet", "X400", "Pref", NULL },
	(gchar *[]){ "ADR", "Dom", "Intl", "Postal", "Parcel", "Home", "Work",
			    "Pref", NULL },
	(gchar *[]){ NULL }
};

static const gchar **
contacts_get_field_types (const gchar *attr_name)
{
	guint i;

	for (i = 0; contacts_field_types[i][0]; i++) {
		if (strcmp (contacts_field_types[i][0], attr_name) == 0)
			return (const gchar **)contacts_field_types[i];
	}
	
	return NULL;
}

static const ContactsStructuredField *
contacts_get_structured_field_name (const gchar *attr_name, guint field)
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

static guint
contacts_get_structured_field_size (const gchar *attr_name)
{
	guint i, size = 1;
	
	for (i = 0; contacts_sfields[i].attr_name; i++)
		if (strcmp (contacts_sfields[i].attr_name, attr_name) == 0)
			if (contacts_sfields[i].field+1 > size)
				size = contacts_sfields[i].field+1;
	
	return size;
}

static const ContactsField *
contacts_get_contacts_field (const gchar *vcard_field)
{
	guint i;
	
	for (i = 0; contacts_fields[i].vcard_field != NULL; i++) {
		if (strcmp (contacts_fields[i].vcard_field, vcard_field) == 0)
			return &contacts_fields[i];
	}
	
	return NULL;
}

static const gchar *
contacts_field_pretty_name (const ContactsField *field)
{
	if (field->pretty_name) {
		return field->pretty_name;
	} else if (field->econtact_field > 0) {
		return e_contact_pretty_name (field->econtact_field);
	} else
		return NULL;
}

static EVCardAttribute *
contacts_add_attr (EVCard *contact, const gchar *vcard_field)
{
	const ContactsField *field = contacts_get_contacts_field (vcard_field);
	
	if (field) {
		guint j;
		/* Create missing attribute */
		EVCardAttribute *attr = e_vcard_attribute_new (
				"", vcard_field);
		/* Initialise values */
		for (j = contacts_get_structured_field_size (
		     e_vcard_attribute_get_name (attr));
		     j > 0; j--)
			e_vcard_attribute_add_value (attr, "");
		/* Add to contact */
		e_vcard_add_attribute (contact, attr);
		
		return attr;
	}
	
	return NULL;
} 

static void
contacts_add_field (GtkWidget *button, EVCard *contact)
{
	GList *fields = NULL;
	GHashTable *field_trans;
	GList *field;
	guint i;
	GladeXML *xml = glade_get_widget_tree (button);
	
	field_trans = g_hash_table_new (g_str_hash, g_str_equal);
	for (i = 0; contacts_fields[i].vcard_field; i++) {
		const gchar *vcard_field = contacts_fields[i].vcard_field;
		const gchar *pretty_name = contacts_field_pretty_name (
						&contacts_fields[i]);
		fields = g_list_append (fields, (gpointer)pretty_name);
		g_hash_table_insert (field_trans, (gpointer)vcard_field,
				     (gpointer)pretty_name);
	}
	
	if (contacts_chooser (xml, "Add field",
			      "<span><b>Choose a field</b></span>", fields,
			      NULL, FALSE, &field)) {
		GtkWidget *label, *edit;
		EVCardAttribute *attr =
			contacts_add_attr (contact, (const gchar *)field->data);
		
/*		const gchar *pretty_name = contacts_field_pretty_name (
						&contacts_fields[i]);

		label = contacts_label_widget_new (contact, attr,
			   pretty_name, contacts_fields[i].multi_line);
		edit = contacts_edit_widget_new (contact, attr,
					contacts_fields[i].multi_line);*/
		
		g_list_free (field);
	}
	
	g_list_free (fields);
	g_hash_table_destroy (field_trans);
}

static void
contacts_remove_edit_widgets_cb (GtkWidget *widget, gpointer data)
{
	gtk_container_remove (GTK_CONTAINER (data), widget);
}

static void
contacts_edit_pane_hide (GtkWidget *button, ContactsData *data)
{
	GtkWidget *widget;
	
	/* Clean contact */
	contacts_clean_contact (data->contact);
	
	/* Commit changes */
	if (data->contact)
		e_book_commit_contact(data->book, data->contact, NULL);

	/* All changed are instant-apply, so just remove the edit components
	 * and switch back to main view.
	 */
	widget = glade_xml_get_widget (data->xml, "edit_table");
	gtk_container_foreach (GTK_CONTAINER (widget),
			       (GtkCallback)contacts_remove_edit_widgets_cb,
			       widget);
	widget = glade_xml_get_widget (data->xml, "main_notebook");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 0);
	widget = glade_xml_get_widget (data->xml, "main_window");
	gtk_window_set_title (GTK_WINDOW (widget), "Contacts");
	contact_selected_sensitive (data->xml, TRUE);
}

static void
contacts_type_entry_changed (GtkWidget *widget, EContactTypeChangeData *data)
{
	GList *v = contacts_entries_get_values (widget, NULL);
	
	if (v) {
		guint i;
		const gchar **types =
			contacts_get_field_types (data->attr_name);
		const gchar *type = (const gchar *)v->data;
		gboolean custom_type = TRUE;
		
		for (i = 0; types[i]; i++) {
			if (strcasecmp (type, types[i]) == 0)
				custom_type = FALSE;
		}
		
		e_vcard_attribute_param_remove_values (data->param);
		/* Prefix a 'X-' to non-standard types, as per vCard spec */
		if (custom_type) {
			gchar *new_type = g_strdup_printf ("X-%s", type);
			e_vcard_attribute_param_add_value (
				data->param, (const char *)new_type);
			g_free (new_type);
		} else
			e_vcard_attribute_param_add_value (
				data->param, (const char *)v->data);
		g_list_foreach (v, (GFunc)g_free, NULL);
		g_list_free (v);
	}
}

static void
contacts_entry_changed (GtkWidget *widget, EContactChangeData *data)
{
	GList *v, *values = contacts_entries_get_values (data->widget, NULL);
	
	e_vcard_attribute_remove_values (data->attr);
	for (v = g_list_last (values); v; v = v->prev) {
		e_vcard_attribute_add_value (data->attr,
					     (const gchar *)v->data);
	}
	g_list_foreach (values, (GFunc)g_free, NULL);
	g_list_free (values);
}

static GtkWidget *
contacts_type_edit_widget_new (EVCardAttribute *attr, gboolean multi_line)
{
	const gchar **types;

	types = contacts_get_field_types (e_vcard_attribute_get_name (attr));
	if (types) {
		guint i;
		GtkWidget *combo = gtk_combo_box_entry_new_text ();
		GtkWidget *align;
		gchar *first_type = "";
		EVCardAttributeParam *param;
		EContactTypeChangeData *data;
		/* Retrieve all types, but we only look at the first one
		 * TODO: A sane way of selecting multiple types, to conform
		 * with spec? (Almost no other vCard-using apps do this)
		 */
		GList *contact_types = contacts_get_types (
			e_vcard_attribute_get_params (attr));
			
		if (contact_types) {
			param = (EVCardAttributeParam *)contact_types->data;
			GList *values =
				e_vcard_attribute_param_get_values (param);
			first_type = values ? (gchar *)values->data : "";
			g_list_free (contact_types);
		} else {
			param = e_vcard_attribute_param_new ("TYPE");
			e_vcard_attribute_add_param_with_value (
				attr, param, "");
		}
		data = g_new (EContactTypeChangeData, 1);
		data->param = param;
		data->attr_name = e_vcard_attribute_get_name (attr);
		
		for (i = 1; types[i]; i++) {
			gtk_combo_box_append_text (
				GTK_COMBO_BOX (combo), types[i]);
			/* Note: We use a case-insensitive search here, as
			 * specified in the spec (as the types are predefined,
			 * we can use non-locale-friendly strcasecmp)
			 */
			if (first_type) {
				if (strcasecmp (types[i], first_type) == 0) {
					first_type = NULL;
					gtk_combo_box_set_active (
						GTK_COMBO_BOX (combo), i-1);
				}
			}
		}
		/* Support custom types, as per spec */
		if ((first_type) && (strncasecmp (first_type, "X-", 2) == 0)) {
			gtk_entry_set_text (
				GTK_ENTRY (GTK_BIN (combo)->child),
				(const gchar *)(first_type+2));
			first_type = NULL;
		}
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "Other");
		if (first_type)
			gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i-1);
		gtk_widget_show (combo);
		if (/*(e_vcard_attribute_is_single_valued (attr)) &&*/
		    (multi_line == FALSE))
			align = gtk_alignment_new (0, 0.5, 0, 0);
		else
			align = gtk_alignment_new (0, 0, 0, 0);
		/* TODO: Find something better than this? */
		gtk_widget_set_size_request (combo, 96, -1);
		gtk_container_add (GTK_CONTAINER (align), combo);

		/* Connect signal for changes */		
		g_signal_connect (G_OBJECT (combo), "changed",
				  G_CALLBACK (contacts_type_entry_changed),
				  data);
		
		/* Free change data structure on destruction */
		g_signal_connect_swapped (G_OBJECT (align), "destroy", 
					  G_CALLBACK (g_free), data);
				  
		return align;
	}
	
	return NULL;
}

/* This function returns a GtkEntry derived from field_id for a particular
 * contact.
 * Returns GtkWidget * on success, NULL on failure
 */
static GtkWidget *
contacts_label_widget_new (EContact *contact, EVCardAttribute *attr,
			   const gchar *name, gboolean multi_line)
{	
	GtkWidget *label_widget, *type_edit = NULL, *container;
	gchar *label_markup;

	/* Create label/button text */
	label_markup = g_strdup_printf ("<span><b>%s:</b></span>", name);

	/* Create widget */
	label_widget = gtk_label_new (NULL);
	gtk_label_set_use_markup (GTK_LABEL (label_widget), TRUE);
	gtk_label_set_markup (GTK_LABEL (label_widget), label_markup);
	gtk_label_set_justify (GTK_LABEL (label_widget), GTK_JUSTIFY_RIGHT);
	if (/*(e_vcard_attribute_is_single_valued (attr)) &&*/
	    (multi_line == FALSE))
		gtk_misc_set_alignment (GTK_MISC (label_widget), 1, 0.5);
	else
		gtk_misc_set_alignment (GTK_MISC (label_widget), 1, 0);

	g_free (label_markup);
	
	if (e_vcard_attribute_is_single_valued (attr))
		type_edit = contacts_type_edit_widget_new (attr, multi_line);
	if (type_edit) {
		gtk_widget_show (type_edit);
		gtk_widget_show (label_widget);
		container = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (container), label_widget,
				    FALSE, TRUE, 0);
		gtk_box_pack_end (GTK_BOX (container), type_edit,
				    FALSE, TRUE, 0);
		label_widget = container;
	}
	
	gtk_widget_set_name (label_widget, e_vcard_attribute_get_name (attr));
	return label_widget;
}

/* This function returns a GtkEntry derived from field_id for a particular
 * contact.
 * Returns GtkWidget * on success, NULL on failure
 * TODO: Lots of duplicated code here, perhaps add a few bits to utils?
 */
static GtkWidget *
contacts_edit_widget_new (EContact *contact, EVCardAttribute *attr,
			  gboolean multi_line)
{	
	EContactChangeData *data;
	const gchar *attr_name = e_vcard_attribute_get_name (attr);

	/* Create data structure for changes */
	data = g_new0 (EContactChangeData, 1);
	data->contact = contact;
	data->attr = attr;

	/* Create widget */
	if (!e_vcard_attribute_is_single_valued (attr)) {
		/* Handle structured fields */
		GtkWidget *adr_table, *type_edit;
		guint field;
		GList *values = e_vcard_attribute_get_values (attr);
		
		adr_table = gtk_table_new (1, 2, FALSE);
		gtk_table_set_col_spacings (GTK_TABLE (adr_table), 6);
		gtk_table_set_row_spacings (GTK_TABLE (adr_table), 6);
		gtk_container_set_border_width (GTK_CONTAINER (adr_table), 6);
		
		/* Set widget that contains attribute data */		
		data->widget = adr_table;
		
		/* Add type editor (due to GtkComboBoxEntry not working and
		 * looking very ugly in GtkExpander)
		 */
		type_edit = contacts_type_edit_widget_new (attr, multi_line);
		if (type_edit) {
			GtkWidget *label = gtk_label_new (NULL);
			gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
			gtk_label_set_markup (GTK_LABEL (label),
				"<span><b>Type:</b></span>");
			gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
			gtk_table_attach (GTK_TABLE (adr_table), label, 0, 1,
					  0, 1, GTK_FILL, GTK_FILL, 0, 0);
			gtk_table_attach (GTK_TABLE (adr_table), type_edit,
					  1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
			gtk_widget_show (label);
			gtk_widget_show (type_edit);
		}

		/* Create sub-fields */
		for (field = 0; values; values = values->next, field++) {
			/* If no information exists, assume field is
			 * single-lined.
			 * TODO: It may be more intelligent in the future
			 * to look at the value and search for new-line
			 * characters.
			 */
			gboolean multiline = FALSE;
			GtkWidget *entry;
			const ContactsStructuredField *sfield =
			    contacts_get_structured_field_name (attr_name,
			    					field);
			const gchar *string = (const gchar *)values->data;
			/* If we have the information, label the field */
			if (sfield) {
				GtkWidget *label = gtk_label_new (NULL);
				gchar *label_markup;

				multiline = sfield->multiline;				
				gtk_label_set_use_markup (GTK_LABEL (label),
							  TRUE);
				label_markup = g_strdup_printf (
					"<span><b>%s:</b></span>",
					sfield->subfield_name);
				gtk_label_set_markup (GTK_LABEL (label),
						      label_markup);
				g_free (label_markup);
				gtk_label_set_justify (GTK_LABEL (label),
						       GTK_JUSTIFY_RIGHT);
				if (!multiline)
					gtk_misc_set_alignment (GTK_MISC (label),
								1, 0.5);
				else
					gtk_misc_set_alignment (GTK_MISC (label),
								1, 0);
				gtk_table_attach (GTK_TABLE (adr_table), label,
						  0, 1, field + 1, field + 2,
						  GTK_FILL, GTK_FILL, 0, 0);
				gtk_widget_show (label);				
			}
			/* This code is pretty much a verbatim copy of the
			 * code below to handle single-valued fields
			 */
			if (multiline) {
				GtkTextBuffer *buffer;
				GtkTextView *view;
				
				view = GTK_TEXT_VIEW (gtk_text_view_new ());
				buffer = gtk_text_view_get_buffer (view);
				
				gtk_text_buffer_set_text (buffer, string ?
							   string : "", -1);
				gtk_text_view_set_editable (view, TRUE);
				
				entry = gtk_scrolled_window_new (NULL, NULL);
				gtk_scrolled_window_set_policy (
					GTK_SCROLLED_WINDOW (entry),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
				gtk_scrolled_window_set_shadow_type (
				   GTK_SCROLLED_WINDOW (entry), GTK_SHADOW_IN);
				gtk_container_add (GTK_CONTAINER (entry),
						   GTK_WIDGET (view));
				gtk_widget_show (GTK_WIDGET (view));
				
				/* Connect signal for changes */
				g_signal_connect (G_OBJECT (buffer), "changed",
						  G_CALLBACK
						  (contacts_entry_changed),
						  data);
			} else {
				entry = gtk_entry_new ();
				gtk_entry_set_text (GTK_ENTRY (entry),
						    string ? string : "");

				/* Connect signal for changes */
				g_signal_connect (G_OBJECT (entry), "changed",
						  G_CALLBACK
						  (contacts_entry_changed),
						  data);
			}
			gtk_widget_show (entry);
			gtk_table_attach (GTK_TABLE (adr_table), entry,
					  1, 2, field + 1, field + 2,
					  GTK_FILL | GTK_EXPAND, GTK_FILL,
					  0, 0);
		}
	} else if (multi_line) {
		/* Handle single-valued fields that span multiple lines */
		const gchar *string = e_vcard_attribute_get_value (attr);
		
		GtkWidget *window;
		GtkTextView *view = GTK_TEXT_VIEW (gtk_text_view_new ());
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
		
		gtk_text_buffer_set_text (buffer, string ? string : "", -1);
		gtk_text_view_set_editable (view, TRUE);
		
		window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (
			GTK_SCROLLED_WINDOW (window),
			GTK_POLICY_AUTOMATIC,
			GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (
			GTK_SCROLLED_WINDOW (window), GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (window),
				   GTK_WIDGET (view));
		gtk_widget_show (GTK_WIDGET (view));

		/* Set widget that contains attribute data */		
		data->widget = window;
		
		/* Connect signal for changes */
		g_signal_connect (G_OBJECT (buffer), "changed", G_CALLBACK
				  (contacts_entry_changed), data);
	} else {
		/* Handle simple single-valued single-line fields */
		const gchar *string = e_vcard_attribute_get_value (attr);
		GtkWidget *entry;

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry),
				    string ? string : "");

		/* Set widget that contains attribute data */		
		data->widget = entry;
		
		/* Connect signal for changes */
		g_signal_connect (G_OBJECT (entry), "changed", G_CALLBACK
				  (contacts_entry_changed), data);
	}

	/* Free change data structure on destruction */
	g_signal_connect_swapped (G_OBJECT (data->widget), "destroy", 
				  G_CALLBACK (g_free), data);
	
	gtk_widget_set_name (data->widget, attr_name);
	return data->widget;
}

static gint
contacts_widgets_list_sort (GtkWidget *a, GtkWidget *b)
{
	const ContactsField *field1, *field2;
	
	field1 = contacts_get_contacts_field (gtk_widget_get_name (a));
	field2 = contacts_get_contacts_field (gtk_widget_get_name (b));
	
	return field1->priority - field2->priority;
}

static gint
contacts_widgets_list_find (GtkWidget *a, guint *b)
{
	const ContactsField *field1;
	
	field1 = contacts_get_contacts_field (gtk_widget_get_name (a));
	
	return field1->priority - *b;
}

void
contacts_edit_pane_show (ContactsData *data)
{
	GtkWidget *button, *widget;
	guint row, i;
	GList *attributes, *c, *d, *label_widgets, *edit_widgets;
	EContact *contact = data->contact;
	GladeXML *xml = data->xml;
	
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
	
	/* Display edit pane */
	/* ODD: Doing this after adding the widgets will cause the first view
	 * not to work... But only when using a viewport (Gtk bug?)
	 */
	widget = glade_xml_get_widget (xml, "main_notebook");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 1);
	
	/* Create contact photo button */
	button = gtk_button_new ();
	widget = GTK_WIDGET (contacts_load_photo (contact));
	gtk_container_add (GTK_CONTAINER (button), widget);
	gtk_widget_show (widget);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (contacts_choose_photo), contact);
	gtk_widget_show (button);
	
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
				contacts_field_pretty_name (field);
			
			label = contacts_label_widget_new (contact, a,
							   pretty_name,
							   field->multi_line);
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
	
	/* Add any missing widgets */
	for (i = 0; contacts_fields[i].vcard_field != NULL; i++) {
		if (contacts_fields[i].priority >= REQUIRED)
			continue;
		if (g_list_find_custom (label_widgets,
					&contacts_fields[i].priority,
					(GCompareFunc)
					 contacts_widgets_list_find) == NULL) {
			EVCardAttribute *attr = 
				contacts_add_attr (&contact->parent,
					   contacts_fields[i].vcard_field);
			GtkWidget *label, *edit;
			const gchar *pretty_name = contacts_field_pretty_name (
							&contacts_fields[i]);

			label = contacts_label_widget_new (contact, attr,
				   pretty_name, contacts_fields[i].multi_line);
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
	
	/* Sort widgets into order and display */
	widget = glade_xml_get_widget (xml, "edit_table");
	g_list_sort (label_widgets, (GCompareFunc)contacts_widgets_list_sort);
	g_list_sort (edit_widgets, (GCompareFunc)contacts_widgets_list_sort);
	i = 2;
	for (c = label_widgets, d = edit_widgets, row = 0; (c) && (d);
	     c = c->next, d = d->next, row++) {
		if (row == 2) {
/*			GtkWidget *hr = gtk_hseparator_new ();
	     		gtk_table_attach (GTK_TABLE (widget), hr, 0, 2,
	     				  3, 4, GTK_FILL, 0, 0, 6);
	     		gtk_widget_show (hr);
	     		row ++;*/
	     		/* Add photo button */
			gtk_table_attach (GTK_TABLE (widget), button, 2, 3,
					  0, 2, 0, 0, 0, 0);
	     		i = 3;
	     	}
	     	
		gtk_widget_show (GTK_WIDGET (c->data));
		gtk_widget_show (GTK_WIDGET (d->data));
	     	if (contacts_get_structured_field_name (
	     	    gtk_widget_get_name (GTK_WIDGET (c->data)), 0)) {
	     		GtkWidget *expander = gtk_expander_new (NULL);
	     		GtkWidget *viewport = gtk_viewport_new (NULL, NULL);
	     		gtk_expander_set_label_widget (GTK_EXPANDER (expander),
	     					       GTK_WIDGET (c->data));
	     		gtk_container_add (GTK_CONTAINER (viewport),
	     				   GTK_WIDGET (d->data));
	     		gtk_widget_show (GTK_WIDGET (viewport));
	     		gtk_container_add (GTK_CONTAINER (expander), viewport);
	     		gtk_expander_set_expanded (GTK_EXPANDER (expander),
	     					   TRUE);
	     		gtk_widget_show (GTK_WIDGET (expander));
	     		gtk_table_attach (GTK_TABLE (widget),
	     				  GTK_WIDGET (expander), 0, 3,
	     				  row, row+1, GTK_FILL | GTK_EXPAND,
	     				  GTK_FILL, 0, 0);
	     	} else {
			gtk_table_attach (GTK_TABLE (widget), GTK_WIDGET (c->data),
					  0, 1, row, row+1,
					  GTK_FILL, GTK_FILL, 0, 0);
			gtk_table_attach (GTK_TABLE (widget),
					  GTK_WIDGET (d->data), 1, i, row,
					  row+1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
		}
	}
	g_list_free (label_widgets);
	g_list_free (edit_widgets);

	widget = glade_xml_get_widget (xml, "main_window");
	gtk_window_set_title (GTK_WINDOW (widget), "Edit contact");
	
	/* Connect add field button */
	widget = glade_xml_get_widget (xml, "add_field_button");
	g_signal_connect (G_OBJECT (widget), "clicked", 
			  G_CALLBACK (contacts_add_field), &contact->parent);
	
	/* Connect close button */
	widget = glade_xml_get_widget (xml, "edit_done_button");
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (contacts_edit_pane_hide), data);
}
