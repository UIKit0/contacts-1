#include <string.h>

#include "contacts-main.h"
#include "contacts-callbacks-ui.h"
#include "contacts-ui.h"
#include "contacts-utils.h"
#include "contacts-edit-pane.h"

#define CONTACTS_VIEW_PAGE 0
#define CONTACTS_EDIT_PAGE 1


void
contacts_remove_labels_from_focus_chain (GtkContainer *container)
{
	GList *chain, *l;
	
	gtk_container_get_focus_chain (container, &chain);
	
	for (l = chain; l; l = l->next) {
		if (GTK_IS_LABEL (l->data)) {
			gconstpointer data = l->data;
			l = l->prev;
			chain = g_list_remove (chain, data);
		}
	}
	
	gtk_container_set_focus_chain (container, chain);
	g_list_free (chain);
}


void
contacts_display_summary (EContact *contact, ContactsData *data)
{
	GtkWidget *widget;
	const gchar *string;
	GtkImage *photo;
	GList *a, *groups, *attributes;
	gchar *name_markup, *groups_text = NULL;
	GValue *can_focus = g_new0 (GValue, 1);

	if (gtk_notebook_get_current_page (GTK_NOTEBOOK (data->ui->main_notebook))
			== CONTACTS_EDIT_PAGE)
		contacts_edit_pane_hide (data);

	if (!E_IS_CONTACT (contact))
		return;

	/* Retrieve contact name and groups */
	widget = data->ui->summary_name_label;
	string = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
	/* Only examine 4-bytes (maximum UTF-8 character width is 4 bytes?) */
	if ((!string) || (g_utf8_strlen (string, 4) <= 0))
		string = _("Unnamed");
	groups = e_contact_get (contact, E_CONTACT_CATEGORY_LIST);
	groups_text = contacts_string_list_as_string (groups, ", ", FALSE);
	if (groups_text)
		name_markup = g_markup_printf_escaped ("<big><b>%s</b></big>\n<small>%s</small>", string ? string : "", groups_text);
	else
		name_markup = g_markup_printf_escaped ("<big><b>%s</b></big>", string ? string : "");

	gtk_label_set_markup (GTK_LABEL (widget), name_markup);
	if (groups) {
		g_list_foreach (groups, (GFunc) g_free, NULL);
		g_list_free (groups);
		g_free (groups_text);
	}
	g_free (name_markup);

	/* Retrieve contact picture and resize */
	widget = data->ui->photo_image;
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

	/* Create summary (displays fields marked as REQUIRED) */
	widget = data->ui->summary_table;
	gtk_container_foreach (GTK_CONTAINER (widget),
			       (GtkCallback)contacts_remove_edit_widgets_cb,
			       widget);
	g_object_set (widget, "n-rows", 1, NULL);
	g_object_set (widget, "n-columns", 2, NULL);
	attributes = e_vcard_get_attributes (E_VCARD (contact));
	g_value_init (can_focus, G_TYPE_BOOLEAN);
	g_value_set_boolean (can_focus, FALSE);
	for (a = attributes; a; a = a->next) {
		GtkWidget *name_widget, *value_widget;
		gchar *value_text, *name_markup;
		GList *values;
		const gchar **types;
		const gchar *attr_name;
		EVCardAttribute *attr = (EVCardAttribute *)a->data;
		const ContactsField *field = contacts_get_contacts_field (
			e_vcard_attribute_get_name (attr));
			
		if (!field || field->priority >= REQUIRED)
			continue;
		
		values = e_vcard_attribute_get_values (attr);
		value_text =
			contacts_string_list_as_string (values, "\n", FALSE);
		
		attr_name = e_vcard_attribute_get_name (attr);

		/* we already have the Full Name above ... */
		if (!strcmp (attr_name, "FN"))
		{
			g_free (value_text);
			continue;
		}

		types = contacts_get_field_types (attr_name);
		if (types) {
			gchar *types_string = NULL;
			const gchar **valid_types;
			GList *types_list = NULL;
			guint i;

			types_list = contacts_get_type_strings (
				e_vcard_attribute_get_params (attr));
			if (types_list) {
				valid_types =
					contacts_get_field_types (attr_name);
				if (g_ascii_strncasecmp (types_list->data,
				    "X-", 2) == 0)
					types_string = (gchar *)
							(types_list->data)+2;
				else if (valid_types) {
					for (i = 0; valid_types[i]; i++) {
						if (g_ascii_strcasecmp (
							types_list->data,
							valid_types[i]) == 0) {
							types_string = (gchar *)
								valid_types[i];
							break;
						}
					}
				}
			}
			if (!types_string)
				types_string = _("Other");
			
			name_markup = g_strdup_printf (
			//	"<b>%s:</b>\n<small>(%s)</small>",
				"<b>%s:</b>",
			//	contacts_field_pretty_name (field),
				types_string);
				
			g_list_free (types_list);
		} else {
			name_markup = g_strdup_printf ("<b>%s:</b>",
				contacts_field_pretty_name (field));
		}
		name_widget = gtk_label_new (name_markup);
		gtk_label_set_use_markup (GTK_LABEL (name_widget), TRUE);
		value_widget = gtk_label_new (value_text);
		gtk_label_set_selectable (GTK_LABEL (value_widget), TRUE);
		gtk_label_set_justify (GTK_LABEL (name_widget),
				       GTK_JUSTIFY_RIGHT);
		gtk_misc_set_alignment (GTK_MISC (name_widget), 1, 0);
		gtk_misc_set_alignment (GTK_MISC (value_widget), 0, 0);
/*		g_object_set_property (G_OBJECT (name_widget),
			"can-focus", can_focus);
		g_object_set_property (G_OBJECT (value_widget),
			"can-focus", can_focus);*/
		
		contacts_append_to_edit_table (GTK_TABLE (widget), name_widget,
					       value_widget, FALSE);
		
		g_free (name_markup);
		g_free (value_text);
	}
	g_value_unset (can_focus);
	g_free (can_focus);

	widget = data->ui->summary_vbox;
	gtk_widget_show (widget);
	contacts_set_available_options (data, TRUE, TRUE, TRUE);

	widget = data->ui->summary_table;
	contacts_remove_labels_from_focus_chain (GTK_CONTAINER (widget));
}

/* Helper method to set edit/delete sensitive/insensitive */
void
contacts_set_available_options (ContactsData *data, gboolean new, gboolean open,
				gboolean delete)
{
	GtkWidget *widget;

	if ((widget = data->ui->new_menuitem))
		gtk_widget_set_sensitive (widget, new);
	if ((widget = data->ui->new_button))
		gtk_widget_set_sensitive (widget, new);

	if ((widget = data->ui->edit_menuitem))
		gtk_widget_set_sensitive (widget, open);
	if ((widget = data->ui->edit_button))
		gtk_widget_set_sensitive (widget, open);

	if ((widget = data->ui->delete_menuitem))
		gtk_widget_set_sensitive (widget, delete);
	if ((widget = data->ui->delete_button))
		gtk_widget_set_sensitive (widget, delete);
}

void
contacts_setup_ui (ContactsData *data)
{


	/* Create model and groups/search filter for contacts list */
	data->contacts_liststore = gtk_list_store_new (3, G_TYPE_STRING,
							G_TYPE_STRING, G_TYPE_STRING);
	data->contacts_filter =
	    GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new
				   (GTK_TREE_MODEL (data->contacts_liststore), NULL));
	gtk_tree_model_filter_set_visible_func (data->contacts_filter,
						(GtkTreeModelFilterVisibleFunc)
						 contacts_is_row_visible_cb,
						data->contacts_table,
						NULL);


	/* Alphabetise the list */

	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (data->contacts_liststore),
						 contacts_sort_treeview_cb,
						 NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (data->contacts_liststore),
				      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
				      GTK_SORT_ASCENDING);
	//g_object_unref (data->contacts_liststore);

	/* these are defined in the frontend header */
	/* create the ui */
	contacts_ui_create (data);

	/* Set transient parent for chooser */
	if (data->ui->chooser_dialog)
	{
		gtk_window_set_transient_for (
			GTK_WINDOW (data->ui->chooser_dialog),
			GTK_WINDOW (data->ui->main_window));
	}

	/* Remove selectable label from focus chain */
	contacts_remove_labels_from_focus_chain (GTK_CONTAINER (data->ui->preview_header_hbox));

}
