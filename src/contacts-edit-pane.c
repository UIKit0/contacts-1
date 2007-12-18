/* 
 *  Contacts - A small libebook-based address book.
 *
 *  Authored By Chris Lord <chris@o-hand.com>
 *
 *  Copyright (c) 2005 OpenedHand Ltd - http://o-hand.com
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

#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>

#include "contacts-defs.h"
#include "contacts-main.h"
#include "contacts-utils.h"
#include "contacts-ui.h"
#include "contacts-callbacks-ui.h"
#include "contacts-callbacks-ebook.h"


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

void
contacts_remove_edit_widgets_cb (GtkWidget *widget, gpointer data)
{
	gtk_container_remove (GTK_CONTAINER (data), widget);
}

static void
contacts_edit_pane_hide (ContactsData *data)
{
	GtkWidget *widget;
	GtkWindow *window;

	/* All changed are instant-apply, so just remove the edit components
	 * and switch back to main view.
	 */
	window = GTK_WINDOW (data->ui->main_window);
	widget = data->ui->edit_table;
	gtk_container_foreach (GTK_CONTAINER (widget),
			       (GtkCallback)contacts_remove_edit_widgets_cb,
			       widget);
	if ((widget = data->ui->contact_menu))
		gtk_widget_hide (widget);
	if ((widget = data->ui->contacts_menu))
		gtk_widget_show (widget);
	widget = data->ui->main_notebook;
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 0);
	gtk_window_set_title (window, _("Contacts"));
	contacts_set_available_options (data, TRUE, TRUE, TRUE);
	if (data->ui->edit_button)
		gtk_window_set_default (window, data->ui->edit_button);
	if ((widget = data->ui->search_entry) && GTK_WIDGET_VISIBLE (widget))
		gtk_window_set_focus (window, data->ui->search_entry);
}

static void
contacts_edit_delete_cb (GtkWidget *button, ContactsData *data)
{
	GtkWidget *dialog, *main_window;
	GList *widgets;
	
	main_window = data->ui->main_window;
	dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
					 0, GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 _("Are you sure you want to delete "\
					 "this contact?"));
	
	widgets = contacts_set_widgets_desensitive (main_window);
	switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
		case GTK_RESPONSE_YES:
			e_book_remove_contact (data->book,
				e_contact_get_const (
					data->contact, E_CONTACT_UID), NULL);
			contacts_set_widgets_sensitive (widgets);
			contacts_edit_pane_hide (data);
			break;
		default:
			contacts_set_widgets_sensitive (widgets);
			break;
	}
	gtk_widget_destroy (dialog);
}

static void
contacts_edit_ok_cb (GtkWidget *button, ContactsData *data)
{
	GError *error = NULL;

	if (data->contact && data->changed) {
		/* Clean contact */
		contacts_clean_contact (data->contact);
	
		/* Commit changes */
		e_book_commit_contact(data->book, data->contact, &error);
		if (error) {
			GtkWidget *dialog;
			GtkWidget *toplevel = gtk_widget_get_toplevel (button);
			dialog = gtk_message_dialog_new (
					GTK_WINDOW (toplevel),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					_("Couldn't update contact")
					);

			gtk_message_dialog_format_secondary_text (
					GTK_MESSAGE_DIALOG (dialog),
					_("Information couldn't be updated, error was: %s"), 
					error->message 
					);

			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);		
			g_warning ("Cannot commit contact: %s", error->message);
			g_error_free (error);
		}
	}

	contacts_edit_pane_hide (data);
}

static void
contacts_edit_ok_new_cb (GtkWidget *button, ContactsData *data)
{
	if (data->contact) {
		/* Clean contact */
		contacts_clean_contact (data->contact);
	
		/* Commit changes */
		if (contacts_contact_is_empty (data->contact)) {
			g_object_unref (data->contact);
		} else {
			GError *error = NULL;
			e_book_add_contact(data->book, data->contact, &error);
			if (error) {
				GtkWidget *dialog;
				GtkWidget *toplevel = gtk_widget_get_toplevel (button);
				dialog = gtk_message_dialog_new (
						GTK_WINDOW (toplevel),
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_OK,
						_("Couldn't add contact")
						);

				gtk_message_dialog_format_secondary_text (
						GTK_MESSAGE_DIALOG (dialog),
						_("Cannot add contact: %s"), 
						error->message 
						);

				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);		
				g_warning ("Cannot add contact: %s",
					error->message);
				g_error_free (error);
			}
		}
	}

	contacts_edit_pane_hide (data);
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
			if (g_ascii_strcasecmp (type, types[i]) == 0)
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
		*data->changed = TRUE;
	}
}

static void
contacts_entry_changed (GtkWidget *widget, EContactChangeData *data)
{
	GList *v, *values = contacts_entries_get_values (data->widget, NULL);
	
	e_vcard_attribute_remove_values (data->attr);
	/* Note: First element of a structured field is type, so remove it */
	if (g_list_length (values) > 1) {
		GList *type = g_list_last (values);
		values = g_list_remove_link (values, type);
		g_free (type->data);
		g_list_free (type);
	}
		
	for (v = g_list_last (values); v; v = v->prev) {
		e_vcard_attribute_add_value (data->attr,
					     (const gchar *)v->data);
	}
	g_list_foreach (values, (GFunc)g_free, NULL);
	g_list_free (values);
	*data->changed = TRUE;
}

static GtkWidget *
contacts_type_edit_widget_new (EVCardAttribute *attr, gboolean multi_line,
	gboolean *changed)
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
		 * with spec? (Almost no other vCard-using apps do this,
		 * so not high priority)
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
		data->changed = changed;
		
		for (i = 0; types[i]; i++) {
			gtk_combo_box_append_text (
				GTK_COMBO_BOX (combo), types[i]);
			/* Note: We use a case-insensitive search here, as
			 * specified in the spec (as the types are predefined,
			 * we can use non-locale-friendly strcasecmp)
			 */
			if (first_type) {
				if (g_ascii_strcasecmp (types[i], first_type) == 0) {
					first_type = NULL;
					gtk_combo_box_set_active (
						GTK_COMBO_BOX (combo), i);
				}
			}
		}
		/* Support custom types, as per spec */
		if ((first_type) && (g_ascii_strncasecmp (first_type, "X-", 2) == 0)) {
			gtk_entry_set_text (
				GTK_ENTRY (GTK_BIN (combo)->child),
				(const gchar *)(first_type+2));
			first_type = NULL;
		}
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Other"));
		if (first_type)
			gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
		gtk_widget_show (combo);
		if (/*(e_vcard_attribute_is_single_valued (attr)) &&*/
		    (multi_line == FALSE))
			align = gtk_alignment_new (0, 0.5, 0, 0);
		else
			align = gtk_alignment_new (0, 0, 0, 0);
		/* TODO: Find something better than this? */
		gtk_widget_set_size_request (combo, 80, -1);
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
GtkWidget *
contacts_label_widget_new (EContact *contact, EVCardAttribute *attr,
			   const gchar *name, gboolean multi_line,
			   gboolean *changed)
{	
	GtkWidget *label_widget, *type_edit = NULL, *container;
	gchar *label_markup;

	/* Create label/button text */
	label_markup = g_strdup_printf (_("<b>%s:</b>"), name);

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
		type_edit = contacts_type_edit_widget_new (attr, multi_line,
			changed);
	if (type_edit) {
		gtk_widget_show (type_edit);
		gtk_widget_show (label_widget);
		container = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (container), label_widget,
				    FALSE, TRUE, 0);
		gtk_box_pack_end (GTK_BOX (container), type_edit,
				    FALSE, FALSE, 0);
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
			  gboolean multi_line, gboolean *changed)
{
	GtkWidget *type_edit;
	EContactChangeData *data;
	const gchar *attr_name = e_vcard_attribute_get_name (attr);

	/* Create data structure for changes */
	data = g_new0 (EContactChangeData, 1);
	data->contact = contact;
	data->attr = attr;
	data->changed = changed;

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
		
		/* Add type editor */
		type_edit = contacts_type_edit_widget_new (attr, multi_line,
			changed);
		if (type_edit) {
			GtkWidget *label = gtk_label_new (NULL);
			gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
			gtk_label_set_markup (GTK_LABEL (label),
                                /* TODO: make nicer for i18n */
				_("<b>Type:</b>"));
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
			GtkWidget *label = NULL, *entry;
			const ContactsStructuredField *sfield =
			    contacts_get_structured_field (attr_name, field);
			const gchar *string = (const gchar *)values->data;
			/* If we have the information, label the field */
			if (sfield) {
				gchar *label_markup;

				label = gtk_label_new (NULL);
				multiline = sfield->multiline;				
				gtk_label_set_use_markup (GTK_LABEL (label),
							  TRUE);

				label_markup = g_strdup_printf (
                                        /** Translators, the first
                                         * argument is the field's
                                         * name, ex. <b>Country:</b>
                                         */				
                                        _("<b>%s:</b>"),
					gettext(sfield->subfield_name));
				gtk_label_set_markup (GTK_LABEL (label),
						      label_markup);
				g_free (label_markup);
				gtk_label_set_justify (GTK_LABEL (label),
						       GTK_JUSTIFY_RIGHT);
				if (!multiline)
					gtk_misc_set_alignment (
						GTK_MISC (label), 1, 0.5);
				else
					gtk_misc_set_alignment (
						GTK_MISC (label), 1, 0);
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
				gtk_widget_set_name (GTK_WIDGET (view),
						     attr_name);
				buffer = gtk_text_view_get_buffer (view);
				
				gtk_text_buffer_set_text (buffer, string ?
							   string : "", -1);
				gtk_text_view_set_editable (view, TRUE);
				
				entry = gtk_frame_new (NULL);
				gtk_frame_set_shadow_type (GTK_FRAME (entry),
							   GTK_SHADOW_IN);
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
				gtk_widget_set_name (entry, attr_name);
				gtk_entry_set_text (GTK_ENTRY (entry),
						    string ? string : "");

				/* Connect signal for changes */
				g_signal_connect (G_OBJECT (entry), "changed",
						  G_CALLBACK
						  (contacts_entry_changed),
						  data);
			}
			gtk_widget_show (entry);
	     		/* Hide the label when the entry is hidden */
			g_signal_connect_swapped (G_OBJECT (entry), "hide", 
					  G_CALLBACK (gtk_widget_hide),
					  label);
			gtk_table_attach (GTK_TABLE (adr_table), entry,
					  1, 2, field + 1, field + 2,
					  GTK_FILL | GTK_EXPAND, GTK_FILL,
					  0, 0);
		}
	} else if (multi_line) {
		/* Handle single-valued fields that span multiple lines */
		gchar *string = e_vcard_attribute_get_value (attr);
		
		GtkWidget *container = NULL;
		GtkTextView *view = GTK_TEXT_VIEW (gtk_text_view_new ());
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
		
		gtk_widget_set_name (GTK_WIDGET (view), attr_name);
		gtk_text_buffer_set_text (buffer, string ? string : "", -1);
		gtk_text_view_set_editable (view, TRUE);
		g_free (string);

		container = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (
			GTK_FRAME (container), GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (container),
				   GTK_WIDGET (view));
		gtk_widget_show (GTK_WIDGET (view));

/*		if (type_edit) {
			gtk_widget_show (type_edit);
			gtk_widget_show (window);
			container = gtk_hbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (container), type_edit,
					    FALSE, TRUE, 0);
			gtk_box_pack_end (GTK_BOX (container), window,
					    TRUE, TRUE, 0);
		}*/

		/* Set widget that contains attribute data */		
		data->widget = container;
		
		/* Connect signal for changes */
		g_signal_connect (G_OBJECT (buffer), "changed", G_CALLBACK
				  (contacts_entry_changed), data);
	} else {
		/* Handle simple single-valued single-line fields */
		gchar *string = e_vcard_attribute_get_value (attr);
		GtkWidget *entry, *container = NULL;

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry),
				    string ? string : "");
		gtk_widget_set_name (entry, attr_name);
		g_free (string);

/*		if (type_edit) {
			gtk_widget_show (type_edit);
			gtk_widget_show (entry);
			container = gtk_hbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (container), type_edit,
					    FALSE, TRUE, 0);
			gtk_box_pack_end (GTK_BOX (container), entry,
					    TRUE, TRUE, 0);
		}*/

		/* Set widget that contains attribute data */		
		data->widget = container ? container : entry;
		
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

static gboolean
contacts_edit_focus_in (GtkWidget *widget, GdkEventFocus *event,
	gpointer user_data)
{
	GtkWidget *ebox = user_data;
/*	GtkWidget *child = gtk_bin_get_child (GTK_BIN (ebox));*/
	
	gtk_widget_set_state (ebox, GTK_STATE_SELECTED);
	
	gtk_widget_set_state (gtk_bin_get_child (
		GTK_BIN (ebox)), GTK_STATE_NORMAL);

/*	if (GTK_IS_EXPANDER (child))
		gtk_widget_set_state (gtk_expander_get_label_widget (
			GTK_EXPANDER (child)), GTK_STATE_SELECTED);
	else if (GTK_IS_LABEL (child))
		gtk_widget_set_state (child, GTK_STATE_SELECTED);
	else if (GTK_IS_BOX (child)) {
		GList *children =
			gtk_container_get_children (GTK_CONTAINER (child));
		if (children && GTK_IS_LABEL (children->data))
			gtk_widget_set_state (
				children->data, GTK_STATE_SELECTED);
		g_list_free (children);
	}*/
	
	return FALSE;
}

static gboolean
contacts_edit_focus_out (GtkWidget *widget, GdkEventFocus *event,
	gpointer user_data)
{
	GtkWidget *ebox = user_data;
	gtk_widget_set_state (ebox, GTK_STATE_NORMAL);
	return FALSE;
}

static GtkWidget *
contacts_edit_add_focus_events (GtkWidget *widget, GtkWidget *ebox,
	GList *widgets)
{
	if (!ebox) {
		GtkWidget *align = gtk_alignment_new (0.5, 0.5, 1, 1);
		ebox = gtk_event_box_new ();
		gtk_container_add (GTK_CONTAINER (align), widget);
		gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 6, 6);
		gtk_container_add (GTK_CONTAINER (ebox), align);
		gtk_widget_show (align);
		gtk_widget_show (ebox);
		gtk_widget_set_name (ebox, gtk_widget_get_name (widget));
	}
	
	if (!widget)
		return ebox;
	
	if (GTK_IS_CONTAINER (widget) && !GTK_IS_TEXT_VIEW (widget)) {
		GList *c, *children =
			gtk_container_get_children (GTK_CONTAINER (widget));
		for (c = children; c; c = c->next) {
			contacts_edit_add_focus_events (GTK_WIDGET (c->data),
				ebox, widgets);
		}
		g_list_free (children);
	} else if (GTK_IS_WIDGET (widget)) {	
		GList *w;
		g_signal_connect (G_OBJECT (widget), "focus-in-event",
			G_CALLBACK (contacts_edit_focus_in), ebox);
		g_signal_connect (G_OBJECT (widget), "focus-out-event",
			G_CALLBACK (contacts_edit_focus_out), ebox);
		for (w = widgets; w; w = w->next) {
			g_signal_connect (G_OBJECT (widget), "focus-in-event",
				G_CALLBACK (contacts_edit_focus_in), w->data);
			g_signal_connect (G_OBJECT (widget), "focus-out-event",
				G_CALLBACK (contacts_edit_focus_out), w->data);
		}
	}
	
	return ebox;
}

/* Helper function to add contacts label/edit widget pairs to a table,
 * with respect for structured field edits.
 */
void
contacts_append_to_edit_table (GtkTable *table, GtkWidget *label,
	GtkWidget *edit, gboolean do_focus)
{
	guint rows, cols;

	g_object_get (table, "n-rows", &rows, NULL);
	g_object_get (table, "n-columns", &cols, NULL);
	
	gtk_widget_show (label);
	gtk_widget_show (edit);
     	if (contacts_get_structured_field (
     	    gtk_widget_get_name (GTK_WIDGET (label)), 0)) {
     		GtkWidget *expander = gtk_expander_new (NULL);
     		GtkWidget *viewport = gtk_viewport_new (NULL, NULL);
     		gtk_expander_set_label_widget (GTK_EXPANDER (expander), label);
     		gtk_container_add (GTK_CONTAINER (viewport), edit);
     		gtk_widget_show (viewport);
     		gtk_container_add (GTK_CONTAINER (expander), viewport);
     		gtk_expander_set_expanded (GTK_EXPANDER (expander),
     					   TRUE);
     		gtk_widget_show (expander);
     		gtk_widget_set_name (expander, gtk_widget_get_name (edit));
     		/* Highlight selected field */
		if (do_focus) {
			expander = contacts_edit_add_focus_events (
				expander, NULL, NULL);
		}
     		gtk_table_attach (table,
     				  GTK_WIDGET (expander), 0, cols,
     				  rows, rows+1, GTK_FILL | GTK_EXPAND,
     				  GTK_FILL, 0, 0);
     	} else {
     		/* Hide the label when the entry is hidden */
		g_signal_connect_swapped (G_OBJECT (edit), "hide", 
					  G_CALLBACK (gtk_widget_hide),
					  label);

     		/* Highlight selected field */
		if (do_focus) {
			GList *l;
			GtkWidget *box = gtk_event_box_new ();
			gtk_container_add (GTK_CONTAINER (box), edit);
			gtk_widget_set_name (box, gtk_widget_get_name (edit));
			gtk_widget_show (box); 
			l = g_list_prepend (NULL, box);
			label = contacts_edit_add_focus_events (
				label, NULL, l);
			g_list_free (l);
			l = g_list_prepend (NULL, label);
			edit = contacts_edit_add_focus_events (edit, box, l);
			g_list_free (l);
		}
		
		gtk_table_attach (table, label,
			0, 1, rows, rows+1, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach (table, edit, 1, cols, rows, rows+1,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
	}
}

typedef struct {
	EVCardAttribute *attr;
	ContactsData *contacts_data;
} ContactsGroupChangeData;

static void
contacts_change_groups_cb (GtkWidget *widget, ContactsGroupChangeData *data)
{
	GList *g, *bools = NULL;
	GList *results = NULL;
	GList *values = e_vcard_attribute_get_values (data->attr);

	for (g = data->contacts_data->contacts_groups; g; g = g->next) {
		if (g_list_find_custom (values, g->data, (GCompareFunc)strcmp))
			bools = g_list_append (bools, GINT_TO_POINTER (TRUE));
		else
			bools = g_list_append (bools, GINT_TO_POINTER (FALSE));
	}
	
	if (contacts_chooser (data->contacts_data, _("Change groups"), _("<b>Choose groups"
		"</b>"), data->contacts_data->contacts_groups, bools, TRUE, TRUE, &results)) {

		gchar *new_groups = results ?
			contacts_string_list_as_string (results, ", ", FALSE) :
			g_strdup ("None");
		g_free (new_groups);

		e_vcard_attribute_remove_values (data->attr);
		for (g = results; g; g = g->next) {
			e_vcard_attribute_add_value (data->attr, g->data);
			if (!g_list_find_custom (data->contacts_data->contacts_groups, g->data, (GCompareFunc) strcmp))
					data->contacts_data->contacts_groups = g_list_prepend (data->contacts_data->contacts_groups, g_strdup (g->data));
		}
		data->contacts_data->changed = TRUE;
	}
}

static gint
contacts_find_widget_cb (GtkWidget *widget, const gchar *name)
{
	return strcmp (gtk_widget_get_name (widget), name);
}

static void
contacts_add_field_cb (GtkWidget *button, ContactsData *data)
{
	GList *fields = NULL;
	GHashTable *field_trans;
	GList *field, *children;
	guint i;
	const ContactsField *contacts_fields = contacts_get_contacts_fields ();
	EContact *contact = data->contact;
	
	children = gtk_container_get_children (GTK_CONTAINER (
		data->ui->edit_table));
	field_trans = g_hash_table_new (g_str_hash, g_str_equal);
	for (i = 0; contacts_fields[i].vcard_field; i++) {
		/* If a unique field exists, don't give the option to add it */
		if (contacts_fields[i].unique) {
			gboolean visible;
			GList *unique_field = g_list_find_custom (children,
				contacts_fields[i].vcard_field,
				(GCompareFunc)contacts_find_widget_cb);
			
			if (unique_field) {
				GObject *f = G_OBJECT (unique_field->data);
				g_object_get (f, "visible", &visible, NULL);
				if (visible)
					continue;
			}
		}
				
		const gchar *vcard_field = contacts_fields[i].vcard_field;
		const gchar *pretty_name = contacts_field_pretty_name (
						&contacts_fields[i]);
		fields = g_list_append (fields, (gpointer)pretty_name);
		g_hash_table_insert (field_trans, (gpointer)pretty_name,
				     (gpointer)vcard_field);
	}
	g_list_free (children);
	
	if (contacts_chooser (data, _("Add field"),
                              /* TODO: make nicer for i18n */
			      _("<b>Choose a field</b>"), fields,
			      NULL, FALSE, FALSE, &field)) {
		GtkWidget *label, *edit, *table;
		const ContactsField *cfield;
		EVCardAttribute *attr;
		const gchar *pretty_name;
		gchar *vcard_field = g_hash_table_lookup (
				      field_trans, (gconstpointer)field->data);
		
		cfield = contacts_get_contacts_field (vcard_field);
		attr = contacts_add_attr (E_VCARD (contact), vcard_field);
		pretty_name = contacts_field_pretty_name (cfield);

		label = contacts_label_widget_new (contact, attr,
			   pretty_name, cfield->multi_line, &data->changed);
		edit = contacts_edit_widget_new (contact, attr,
					cfield->multi_line, &data->changed);
		table = data->ui->edit_table;
		
		contacts_append_to_edit_table (GTK_TABLE (table), label, edit,
			TRUE);
		
		g_list_foreach (field, (GFunc) g_free, NULL);
		g_list_free (field);
	}
	
	g_list_free (fields);
	g_hash_table_destroy (field_trans);
}

/* Helper method to clear the text in a GtkEntry or GtkTextView and hide it */
static void
contacts_remove_entries (GtkWidget *entry)
{
	if (GTK_IS_ENTRY (entry)) {
		if (gtk_widget_get_ancestor (entry, GTK_TYPE_COMBO_BOX_ENTRY))
			return;

		gtk_entry_set_text (GTK_ENTRY (entry), "");
		gtk_widget_hide (entry);
	} else if (GTK_IS_TEXT_VIEW (entry)) {
		GtkTextBuffer *buffer =
			gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry));
			
		gtk_text_buffer_set_text (buffer, "", -1);
		
		entry = gtk_widget_get_ancestor (entry, GTK_TYPE_BIN);
		gtk_widget_hide (entry);
	} else if (GTK_IS_CONTAINER (entry)) {
		GList *c, *children =
			gtk_container_get_children (GTK_CONTAINER (entry));
		
		for (c = children; c; c = c->next)
			contacts_remove_entries (GTK_WIDGET (c->data));
		
		gtk_widget_hide (entry);
		
		g_list_free (children);
	}
}

static gboolean
contacts_widget_is_label (GtkWidget *widget)
{
	if (GTK_IS_LABEL (widget))
		return TRUE;
	else if (GTK_IS_EXPANDER (widget))
		return FALSE;
	else if (GTK_IS_CONTAINER (widget)) {
		GList *c, *children =
			gtk_container_get_children (GTK_CONTAINER (widget));
		
		for (c = children; c; c = c->next) {
			if (contacts_widget_is_label (GTK_WIDGET (c->data))) {
				g_list_free (children);
				return TRUE;
			}
		}
		
		g_list_free (children);
	}
	
	return FALSE;
}

void
contacts_edit_set_focus_cb (GtkWidget *button, GtkWidget *widget,
	GtkWindow *window)
{
	if (widget && (GTK_IS_ENTRY (widget) || GTK_IS_TEXT_VIEW (widget)) &&
	    strcmp ("FN", gtk_widget_get_name (widget)) != 0)
		gtk_widget_set_sensitive (button, TRUE);
	else
		gtk_widget_set_sensitive (button, FALSE);
}

void
contacts_remove_field_cb (GtkWidget *button, gpointer data)
{
	GtkWidget *window = gtk_widget_get_toplevel (button);
	GtkWidget *widget = gtk_window_get_focus (GTK_WINDOW (window));
	GtkWidget *ancestor = gtk_widget_get_ancestor (
		widget, GTK_TYPE_EXPANDER);
	
	gtk_window_set_focus (GTK_WINDOW (window), NULL);

	if (ancestor) {
		contacts_remove_entries (ancestor);
		return;
	}

	ancestor = gtk_widget_get_ancestor (widget, GTK_TYPE_COMBO_BOX_ENTRY);
	if (ancestor) {
		const gchar *name;
		guint top, left;
		GtkWidget *table;
		GList *c, *children;
		
		ancestor = gtk_widget_get_ancestor (
			ancestor, GTK_TYPE_EVENT_BOX);
		name = gtk_widget_get_name (ancestor);
		table = gtk_widget_get_ancestor (ancestor, GTK_TYPE_TABLE);
		gtk_container_child_get (GTK_CONTAINER (table),
			ancestor, "left-attach", &left,
			"top-attach", &top, NULL);
			
		children = gtk_container_get_children (GTK_CONTAINER (table));
		for (c = children; c; c = c->next) {
			guint ctop, cleft;
			gtk_container_child_get (GTK_CONTAINER (table),
				GTK_WIDGET (c->data),
				"left-attach", &cleft,
				"top-attach", &ctop, NULL);
			if ((cleft == left+1) && (ctop == top)) {
				contacts_remove_entries (GTK_WIDGET (c->data));
				break;
			}
		}
		g_list_free (c);
		return;
	}
	
	contacts_remove_entries (widget);
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

static void
contacts_edit_choose_photo (GtkWidget *button, ContactsData *data)
{
	data->changed = TRUE;
	contacts_choose_photo (button, data->contact);
}

void
contacts_edit_pane_show (ContactsData *data, gboolean new)
{
	GtkWidget *align, *button, *widget/*, *glabel, *gbutton*/;
	EVCardAttribute *groups_attr = NULL;
	ContactsGroupChangeData *gdata;
	guint row, i;
	GList *attributes, *c, *d, *label_widgets, *edit_widgets;
	EContact *contact = data->contact;
	const ContactsField *contacts_fields = contacts_get_contacts_fields ();

#ifdef DEBUG
	/* Prints out all contact data */
	attributes = e_vcard_get_attributes (E_VCARD (contact));
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
			EVCardAttributeParam *p =
				(EVCardAttributeParam *)params->data;
			GList *paramvalues =
				e_vcard_attribute_param_get_values (p);
			g_printf ("    %s:\n",
				  e_vcard_attribute_param_get_name (p));
			for (; paramvalues; paramvalues = paramvalues->next) {
				const gchar *value =
					(const gchar *)paramvalues->data;
				g_printf ("      o %s\n", value);
			}
		}
	}
#endif
	/* remove any widgets in the edit table */
	gtk_container_foreach (GTK_CONTAINER (data->ui->edit_table),
			       (GtkCallback)contacts_remove_edit_widgets_cb,
			       data->ui->edit_table);
	
	/* Display edit pane */
	/* ODD: Doing this after adding the widgets will cause the first view
	 * not to work... But only when using a viewport (Gtk bug?)
	 */
	widget = data->ui->main_notebook;
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 1);

	if ((widget = data->ui->contact_menu))
		gtk_widget_show (widget);
	if ((widget = data->ui->contacts_menu))
		gtk_widget_hide (widget);


	/* Create contact photo button */
	button = gtk_button_new ();
	widget = GTK_WIDGET (contacts_load_photo (contact));
	gtk_container_add (GTK_CONTAINER (button), widget);
	gtk_widget_show (widget);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (contacts_edit_choose_photo), data);
	gtk_widget_show (button);
	
	/* Create groups edit label/button */
/*	glabel = gtk_label_new ("_(<b>Groups:</b>)");
	gtk_label_set_use_markup (GTK_LABEL (glabel), TRUE);
	gtk_misc_set_alignment (GTK_MISC (glabel), 1, 0.5);
	gbutton = gtk_button_new_with_label (_("None"));*/
	
	/* Create edit pane widgets */
	attributes = e_vcard_get_attributes (E_VCARD (contact));
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
							   field->multi_line,
							   &data->changed);
			edit = contacts_edit_widget_new (contact, a,
							 field->multi_line,
							 &data->changed);
			
			if (label && edit) {
				label_widgets = g_list_append (label_widgets,
							       label);
				edit_widgets = g_list_append (edit_widgets,
							      edit);
			}
		} else if (g_ascii_strcasecmp (name, "CATEGORIES") == 0) {
			/* Create categories widget */
/*			GList *values = e_vcard_attribute_get_values (a);
			gchar *types =
				contacts_string_list_as_string (values, ", ");
			gtk_button_set_label (GTK_BUTTON (gbutton), types);
			g_free (types);*/
			
			groups_attr = a;
		}
	}
	
	if (!groups_attr) {
		groups_attr = e_vcard_attribute_new (NULL, "CATEGORIES");
		e_vcard_add_attribute (E_VCARD (contact), groups_attr);
	}
	
	/* Add any missing widgets */
	for (i = 0; contacts_fields[i].vcard_field != NULL; i++) {
		if ((contacts_fields[i].priority >= REQUIRED) ||
		    ((!contacts_fields[i].unique) && (!new)))
			continue;
		if (g_list_find_custom (label_widgets,
					&contacts_fields[i].priority,
					(GCompareFunc)
					 contacts_widgets_list_find) == NULL) {
			EVCardAttribute *attr = 
				contacts_add_attr (E_VCARD (contact),
					   contacts_fields[i].vcard_field);
			GtkWidget *label, *edit;
			const gchar *pretty_name = contacts_field_pretty_name (
							&contacts_fields[i]);

			label = contacts_label_widget_new (contact, attr,
				   pretty_name, contacts_fields[i].multi_line,
				   &data->changed);
			edit = contacts_edit_widget_new (contact, attr,
						contacts_fields[i].multi_line,
						&data->changed);
			
			if (label && edit) {
				label_widgets = g_list_append (label_widgets,
							       label);
				edit_widgets = g_list_append (edit_widgets,
							      edit);
			}
		}
	}
	
	/* Sort widgets into order and display */
	widget = data->ui->edit_table;
	label_widgets = g_list_sort (label_widgets,
		(GCompareFunc)contacts_widgets_list_sort);
	edit_widgets = g_list_sort (edit_widgets,
		(GCompareFunc)contacts_widgets_list_sort);
	g_object_set (widget, "n-rows", 1, NULL);
	g_object_set (widget, "n-columns", 2, NULL);
	for (c = label_widgets, d = edit_widgets, row = 0; (c) && (d);
	     c = c->next, d = d->next, row++) {
		/* Make sure address entries are displayed beneath photo */
		if (row < 2 && GTK_IS_CONTAINER (d->data))
			row = 2;
		
		/* Widen panel after photo */
		if (row == 2) {
			g_object_set (widget, "n-columns", 3, NULL);
			g_object_set (widget, "n-rows", 3, NULL);
		}

		contacts_append_to_edit_table (GTK_TABLE (widget),
					       GTK_WIDGET (c->data),
					       GTK_WIDGET (d->data), TRUE);

		/* Set focus on first entry */
		if (row == 0)
			gtk_window_set_focus (GTK_WINDOW (
				data->ui->main_window),
				d->data);
	}
	
	/* Add photo */
	align = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (align);
	gtk_container_add (GTK_CONTAINER (align), button);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 6, 6);
	g_object_set (widget, "n-rows", 3, NULL);
	g_object_set (widget, "n-columns", 3, NULL);
	gtk_table_attach (GTK_TABLE (widget), align, 2, 3,
			  1, 3, 0, 0, 0, 0);
	/* Add groups-editing button */
/*	contacts_append_to_edit_table (GTK_TABLE (widget),
				       glabel, gbutton);*/
				       
	g_list_free (label_widgets);
	g_list_free (edit_widgets);

	widget = data->ui->main_window;
	gtk_window_set_title (GTK_WINDOW (widget), _("Edit contact"));
	
	/* Connect add group menu item */
	widget = data->ui->edit_groups;
	gdata = g_new (ContactsGroupChangeData, 1);
	gdata->contacts_data = data;
	gdata->attr = groups_attr;
	/* Remove any old signals and replace with new ones with the correct
	 * user data.
	 */
	g_signal_handlers_disconnect_matched (G_OBJECT (widget),
					      G_SIGNAL_MATCH_FUNC, 0, 0,
					      NULL, contacts_change_groups_cb,
					      NULL);
	g_signal_handlers_disconnect_matched (G_OBJECT (widget),
					      G_SIGNAL_MATCH_FUNC, 0, 0,
					      NULL, g_free,
					      NULL);
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (contacts_change_groups_cb), gdata);
	g_signal_connect_swapped (G_OBJECT (widget), "hide",
				  G_CALLBACK (g_free), gdata);
	if (data->ui->contacts_menu && data->ui->contact_menu)
	{
		gtk_widget_hide (data->ui->contacts_menu);
		gtk_widget_show (widget);
	}
	if ((widget = data->ui->remove_field_button))
		gtk_widget_set_sensitive (widget, FALSE);

	/* Connect delete menu item */
	widget = data->ui->contact_delete;
	g_signal_handlers_disconnect_matched (G_OBJECT (widget),
					      G_SIGNAL_MATCH_FUNC, 0, 0,
					      NULL, contacts_edit_delete_cb,
					      NULL);
	g_signal_connect (G_OBJECT (widget), "activate", 
			  G_CALLBACK (contacts_edit_delete_cb), data);
			  
	/* Connect export menu item */
	widget = data->ui->contact_export;
	g_signal_handlers_disconnect_matched (G_OBJECT (widget),
					      G_SIGNAL_MATCH_FUNC, 0, 0,
					      NULL, contacts_export_cb,
					      NULL);
	g_signal_connect (G_OBJECT (widget), "activate", 
			  G_CALLBACK (contacts_export_cb), data);
			  
	/* Connect add field button */
	if ((widget = data->ui->add_field_button))
	{
		g_signal_handlers_disconnect_matched (G_OBJECT (widget),
						      G_SIGNAL_MATCH_FUNC, 0, 0,
						      NULL, contacts_add_field_cb,
						      NULL);
		g_signal_connect (G_OBJECT (widget), "clicked", 
				  G_CALLBACK (contacts_add_field_cb), data);
	}
	
	/* Connect close button */
	if ((widget = data->ui->edit_done_button))
	{
		g_signal_handlers_disconnect_matched (G_OBJECT (widget),
						      G_SIGNAL_MATCH_FUNC, 0, 0,
						      NULL, contacts_edit_ok_cb,
						      NULL);
		g_signal_handlers_disconnect_matched (G_OBJECT (widget),
						      G_SIGNAL_MATCH_FUNC, 0, 0,
						      NULL, contacts_edit_ok_new_cb,
						      NULL);
		if (new)
			g_signal_connect (G_OBJECT (widget), "clicked",
					  G_CALLBACK (contacts_edit_ok_new_cb), data);
		else
			g_signal_connect (G_OBJECT (widget), "clicked",
					  G_CALLBACK (contacts_edit_ok_cb), data);
		gtk_window_set_default (GTK_WINDOW (
			data->ui->main_window), widget);
	}
}
