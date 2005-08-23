
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
static ContactsField contacts_fields[] = {
	{ "FN", E_CONTACT_FULL_NAME, NULL, FALSE, 10 },
	{ "TEL", 0, "Phone", FALSE, 20 },
	{ "EMAIL", 0, "Email", FALSE, 30 },
	{ "ADR", 0, "Address", FALSE, 40 },
	{ "NICKNAME", E_CONTACT_NICKNAME, NULL, FALSE, 110 },
	{ "URL", E_CONTACT_HOMEPAGE_URL, NULL, FALSE, 120 },
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
contacts_get_contacts_field_pretty_name (const ContactsField *field)
{
	if (field->pretty_name) {
		return field->pretty_name;
	} else if (field->econtact_field > 0) {
		return e_contact_pretty_name (field->econtact_field);
	} else
		return NULL;
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

/* This function returns a GtkEntry derived from field_id for a particular
 * contact.
 * Returns GtkWidget * on success, NULL on failure
 */
static GtkWidget *
contacts_label_widget_new (EContact *contact, EVCardAttribute *attr,
			   const gchar *name, gboolean multi_line)
{	
	GtkWidget *label_widget;
	GList *types;
	gchar *label_markup;

	/* Create label/button text */
	types = contacts_get_string_list_from_types (
					e_vcard_attribute_get_params (attr));
	if (types) {
		gchar *type_string = 
			contacts_string_list_as_string (types, ", ");
		label_markup = g_strdup_printf (
			"<span><b>%s:</b>\n<small>(%s)</small></span>",
			name, type_string);
		g_free (type_string);
		g_list_free (types);
	}
	else {
		label_markup = g_strdup_printf (
			"<span><b>%s:</b></span>", name);
	}

	/* Create widget */
	label_widget = gtk_label_new (NULL);
	gtk_label_set_use_markup (GTK_LABEL (label_widget), TRUE);
	gtk_label_set_markup (GTK_LABEL (label_widget), label_markup);
	gtk_label_set_justify (GTK_LABEL (label_widget), GTK_JUSTIFY_RIGHT);
	if ((e_vcard_attribute_is_single_valued (attr)) &&
	    (multi_line == FALSE))
		gtk_misc_set_alignment (GTK_MISC (label_widget), 1, 0.5);
	else
		gtk_misc_set_alignment (GTK_MISC (label_widget), 1, 0);

	g_free (label_markup);
	
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
		GtkWidget *adr_table;
		guint field;
		GList *values = e_vcard_attribute_get_values (attr);
		
		adr_table = gtk_table_new (1, 2, FALSE);
		gtk_table_set_col_spacings (GTK_TABLE (adr_table), 6);
		gtk_table_set_row_spacings (GTK_TABLE (adr_table), 6);
		gtk_container_set_border_width (GTK_CONTAINER (adr_table), 6);
		
		/* Set widget that contains attribute data */		
		data->widget = adr_table;
		
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
						  0, 1, field, field + 1,
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
					  1, 2, field, field +1,
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
				contacts_get_contacts_field_pretty_name (field);
			
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
			const gchar *pretty_name =
				contacts_get_contacts_field_pretty_name
				    (contacts_get_contacts_field
					(contacts_fields[i].vcard_field));
			EVCardAttribute *attr = e_vcard_attribute_new (
					"", contacts_fields[i].vcard_field);
			e_vcard_add_attribute (&contact->parent, attr);
			GtkWidget *label, *edit;

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
	     	if (row == 3) i = 3;
		gtk_table_attach (GTK_TABLE (widget), GTK_WIDGET (c->data),
				  0, 1, row, row+1,
				  GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach (GTK_TABLE (widget), GTK_WIDGET (d->data),
				  1, i, row, row+1,
				  GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
		gtk_widget_show (c->data);
		gtk_widget_show (d->data);
	}
	g_list_free (label_widgets);
	g_list_free (edit_widgets);
	gtk_table_attach (GTK_TABLE (widget), button, 2, 3, 0, 3, 0, 0, 0, 0);

	widget = glade_xml_get_widget (xml, "main_window");
	gtk_window_set_title (GTK_WINDOW (widget), "Edit contact");
	
	/* Connect close button */
	widget = glade_xml_get_widget (xml, "edit_done_button");
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (contacts_edit_pane_hide), data);
}
