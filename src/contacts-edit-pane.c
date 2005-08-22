
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


/* List of always-available fields (ADR will have to be special-cased) */
/* TODO: Revise 'supported' fields */
static ContactsField contacts_fields[] = {
	{ "FN", E_CONTACT_FULL_NAME, NULL, FALSE, 10 },
	{ "TEL", 0, "Phone", FALSE, 20 },
	{ "EMAIL", 0, "Email", FALSE, 30 },
	{ "LABEL", 0, "Address", TRUE, 40 },
	{ "NICKNAME", E_CONTACT_NICKNAME, NULL, FALSE, 110 },
	{ "URL", E_CONTACT_HOMEPAGE_URL, NULL, FALSE, 120 },
	{ "NOTE", E_CONTACT_NOTE, NULL, TRUE, 130 },
	{ NULL, 0, NULL, 0 }
};

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
contacts_entry_changed (GtkEntry *entry, gpointer data)
{
	EContactChangeData *d = (EContactChangeData *)data;
	gchar *string;
	string = (gchar *)gtk_entry_get_text (entry);
		
	/* Remove old value and add new one */
	e_vcard_attribute_remove_values (d->attr);
	e_vcard_attribute_add_value (d->attr, string);
}

static void
contacts_buffer_changed (GtkTextBuffer *buffer, gpointer data)
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

static gchar *
contacts_get_type_string (GList *params)
{
	gchar *type_string;
	GList *n, *names = NULL;

	if (!params) return NULL;

	for (; params; params = params->next) {
		EVCardAttributeParam *p = (EVCardAttributeParam *)params->data;
		GList *pvs = e_vcard_attribute_param_get_values (p);
		if (strcmp ("TYPE", e_vcard_attribute_param_get_name (p)) != 0)
			continue;
		for (; pvs; pvs = pvs->next)
			names = g_list_append (names, pvs->data);
	}
	
	if (!names)
		return NULL;
	
	type_string = g_strdup (names->data);
	for (n = names->next; n; n = n->next) {
		gchar *old_type_string = type_string;
		gchar *name_string = g_strdup_printf (", %s", (gchar *)n->data);
		type_string = g_strconcat (type_string, name_string, NULL);
		g_free (old_type_string);
		g_free (name_string);
	}
	g_list_free (names);

	return type_string;
}

/* This function returns a GtkEntry derived from field_id for a particular
 * contact.
 * Returns GtkWidget * on success, NULL on failure
 */
static GtkWidget *
contacts_label_widget_new (EContact *contact, EVCardAttribute *attr,
			   const gchar *name)
{	
	GtkWidget *label_widget;
	gchar *type_string, *label_markup;

	/* Only support single-valued fields (i.e. not ADR) */
	/* This doesn't work when the attributes are new */
/*	if (!e_vcard_attribute_is_single_valued (attr))
		return NULL;*/

	/* Create label/button text */
	type_string =
		contacts_get_type_string (e_vcard_attribute_get_params (attr));
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
	label_widget = gtk_label_new (NULL);
	gtk_label_set_use_markup (GTK_LABEL (label_widget), TRUE);
	gtk_label_set_markup (GTK_LABEL (label_widget), label_markup);
	gtk_label_set_justify (GTK_LABEL (label_widget), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC (label_widget), 1, 0.5);
	gtk_widget_set_name (label_widget, e_vcard_attribute_get_name (attr));

	g_free (label_markup);
	
	return label_widget;
}

/* This function returns a GtkEntry derived from field_id for a particular
 * contact.
 * Returns GtkWidget * on success, NULL on failure
 */
static GtkWidget *
contacts_edit_widget_new (EContact *contact, EVCardAttribute *attr,
			  gboolean multi_line)
{	
	GtkWidget *edit_widget;
	const gchar *string;
	EContactChangeData *data;

	/* Only support single-valued fields (i.e. not ADR) */
	/* This doesn't work when the attributes are new */
/*	if (!e_vcard_attribute_is_single_valued (attr))
		return NULL;*/

	string = e_vcard_attribute_get_value (attr);

	/* Create widget */
	/* Create data structure for changes */
	data = g_new (EContactChangeData, 1);
	data->contact = contact;
	data->attr = attr;

	if (multi_line) {
		GtkTextView *view = GTK_TEXT_VIEW (gtk_text_view_new ());
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
		
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
				  (contacts_buffer_changed), data);
	} else {
		edit_widget = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (edit_widget),
				    string ? string : "");
		
		/* Connect signal for changes */
		g_signal_connect (G_OBJECT (edit_widget), "changed", G_CALLBACK
				  (contacts_entry_changed), data);
	}
	gtk_widget_set_name (edit_widget, e_vcard_attribute_get_name (attr));

	/* Free change data structure on destruction */
	g_signal_connect_swapped (G_OBJECT (edit_widget), "destroy", 
				  G_CALLBACK (g_free), data);
	
	return edit_widget;
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
	GtkWidget *widget;
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
	
	/* Display contact photo */
	widget = glade_xml_get_widget (xml, "edit_photo_button");
	gtk_button_set_image (GTK_BUTTON (widget), 
			      GTK_WIDGET (contacts_load_photo (contact)));
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (contacts_choose_photo), contact);
	
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
	
	/* Sort widgets into order and display */
	widget = glade_xml_get_widget (xml, "edit_table");
	g_list_sort (label_widgets, (GCompareFunc)contacts_widgets_list_sort);
	g_list_sort (edit_widgets, (GCompareFunc)contacts_widgets_list_sort);
	for (c = label_widgets, d = edit_widgets, row = 0; (c) && (d);
	     c = c->next, d = d->next, row++) {
		gtk_table_attach (GTK_TABLE (widget), GTK_WIDGET (c->data),
				  0, 1, row, row+1,
				  GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach (GTK_TABLE (widget), GTK_WIDGET (d->data),
				  1, 2, row, row+1,
				  GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
		gtk_widget_show (c->data);
		gtk_widget_show (d->data);
	}
	g_list_free (label_widgets);
	g_list_free (edit_widgets);

	widget = glade_xml_get_widget (xml, "main_window");
	gtk_window_set_title (GTK_WINDOW (widget), "Edit contact");
	
	/* Connect close button */
	widget = glade_xml_get_widget (xml, "edit_done_button");
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (contacts_edit_pane_hide), data);
}
