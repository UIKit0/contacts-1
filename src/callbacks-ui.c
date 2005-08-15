
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libebook/e-book.h>

#include "globals.h"
#include "defs.h"
#include "utils.h"
#include "callbacks-ui.h"
#include "main.h"

void
quit ()
{
	/* Unload the addressbook and quit */
	e_book_view_stop (book_view);
	g_object_unref (book_view);
	g_object_unref (book);
	gtk_main_quit ();
}

gboolean
is_row_visible (GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	gboolean result = FALSE;
	gchar *group;
	GList *groups, *g;
	const gchar *uid;
	EContactListHash *hash;
	GtkComboBox *groups_combobox;
	const gchar *search_string;

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
	search_string = gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget
							(xml, "search_entry")));
	if ((search_string) && (g_utf8_strlen (search_string, -1) > 0)) {
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
	if (!string1) string1 = g_new0 (gchar, 1);
	if (!string2) string2 = g_new0 (gchar, 1);
	returnval = strcasecmp ((const char *) string1,
				(const char *) string2);
	g_free (string1);
	g_free (string2);

	return returnval;
}

void
update_treeview ()
{
	GtkTreeView *view;
	GtkTreeModelFilter *model;
	gint visible_rows;

	view = GTK_TREE_VIEW (glade_xml_get_widget (xml, "contacts_treeview"));
	model = GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (view));
	visible_rows = 0;
	gtk_tree_model_filter_refilter (model);
	
	visible_rows = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (model),
						       NULL);
	/* If there's only one visible contact, select it */
	if (visible_rows == 1) {
		GtkTreeSelection *selection =
					gtk_tree_view_get_selection (view);
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model),
						   &iter)) {
			gtk_tree_selection_select_iter (selection, &iter);
		}
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

void
display_contact_summary (EContact *contact)
{
	GtkWidget *widget;
	const gchar *string;
	GtkImage *photo;

	if (!E_IS_CONTACT (contact))
		return;

	/* Retrieve contact name */
	widget = glade_xml_get_widget (xml, "summary_name_label");
	string = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
	if (string) {
		gchar *name_markup = g_strdup_printf
		    ("<span><big><b>%s</b></big></span>", string);
		gtk_label_set_markup (GTK_LABEL (widget), name_markup);
		g_free (name_markup);
	} else {
		gtk_label_set_markup (GTK_LABEL (widget), "");
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

void
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
new_contact ()
{
	EContact *contact = e_contact_new ();
	
	if (e_book_add_contact (book, contact, NULL)) {
		do_edit (contact);
	}
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

void
delete_contact ()
{
	GtkWidget *dialog, *main_window;
	EContact *contact;
	gint result;
	
	main_window = glade_xml_get_widget (xml, "main_window");
	dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
					 0, GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 "Are you sure you want to delete "\
					 "this contact?");
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result) {
		case GTK_RESPONSE_YES:
			contact = get_current_contact ();
			if (contact) {
				e_book_remove_contact (book,
					e_contact_get_const
						(contact, E_CONTACT_UID), NULL);
			}
			break;
		default:
			break;
	}
	gtk_widget_destroy (dialog);
}

void
copy (GtkWindow *main_window)
{
	GtkWidget *widget = gtk_window_get_focus (main_window);

	if (widget) {
		if (GTK_IS_EDITABLE (widget))
			gtk_editable_copy_clipboard (GTK_EDITABLE (widget));
		else if (GTK_IS_LABEL (widget)) {
			gint start, end;
			if (gtk_label_get_selection_bounds (GTK_LABEL (widget),
							    &start, &end)) {
				const gchar *text =
					gtk_label_get_text (GTK_LABEL (widget));
				gchar *start_text =
					g_utf8_offset_to_pointer (text, start);
				gchar *copy_text =
					g_strndup (start_text, end-start);
				gtk_clipboard_set_text (
				    gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
				    copy_text, end-start);
				g_free (copy_text);
			}
		}
	}
}

void
cut (GtkWindow *main_window)
{
	GtkWidget *widget = gtk_window_get_focus (main_window);
	
	if (widget && GTK_IS_EDITABLE (widget))
		gtk_editable_cut_clipboard (GTK_EDITABLE (widget));
	else
		copy (main_window);
}

void
paste (GtkWindow *main_window)
{
	GtkWidget *widget = gtk_window_get_focus (main_window);

	if (widget && GTK_IS_EDITABLE (widget))
		gtk_editable_paste_clipboard (GTK_EDITABLE (widget));
}

void
about ()
{
	GtkWidget *widget = glade_xml_get_widget (xml, "aboutdialog");
	gtk_widget_show (widget);
}


static void
remove_edit_components_cb (GtkWidget *widget, gpointer data)
{
	gtk_container_remove (GTK_CONTAINER (data), widget);
}

void
edit_done (GtkWidget *button, EContact *contact)
{
	GtkWidget *widget;
	
	/* Commit changes */
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
