
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libebook/e-book.h>

#include "globals.h"
#include "defs.h"
#include "utils.h"
#include "callbacks-ui.h"
#include "contacts-edit-pane.h"
#include "main.h"

void
contacts_selection_cb (GtkTreeSelection * selection)
{
	GtkWidget *widget;
	EContact *contact;

	/* Get the currently selected contact and update the contact summary */
	contact = get_contact_from_selection (selection);
	if (contact) {
		contacts_display_summary (contact);
	} else {
		contact_selected_sensitive (FALSE);
		widget = glade_xml_get_widget (xml, "summary_vbox");
		gtk_widget_hide (widget);
	}
}

void
contacts_new_cb ()
{
	EContact *contact = e_contact_new ();
	
	if (e_book_add_contact (book, contact, NULL)) {
		contacts_edit_pane_show (contact);
	}
}

void
contacts_edit_cb ()
{
	EContact *contact;

	/* Disable the edit/delete buttons and get the contact to edit */
	contact_selected_sensitive (FALSE);
	contact = get_current_contact ();
	
	contacts_edit_pane_show (contact);
}

void
contacts_delete_cb ()
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
contacts_copy_cb (GtkWindow *main_window)
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
contacts_cut_cb (GtkWindow *main_window)
{
	GtkWidget *widget = gtk_window_get_focus (main_window);
	
	if (widget && GTK_IS_EDITABLE (widget))
		gtk_editable_cut_clipboard (GTK_EDITABLE (widget));
	else
		contacts_copy_cb (main_window);
}

void
contacts_paste_cb (GtkWindow *main_window)
{
	GtkWidget *widget = gtk_window_get_focus (main_window);

	if (widget && GTK_IS_EDITABLE (widget))
		gtk_editable_paste_clipboard (GTK_EDITABLE (widget));
}

void
contacts_about_cb (GtkWidget *dialog)
{
	gtk_widget_show (dialog);
}

gboolean
contacts_is_row_visible_cb (GtkTreeModel * model, GtkTreeIter * iter,
			    gpointer data)
{
	gboolean result = FALSE;
	gchar *group;
	GList *groups, *g;
	const gchar *uid;
	EContactListHash *hash;
	GtkComboBox *groups_combobox;
	const gchar *search_string;

	/* Check if the contact is in the currently selected group. */
	gtk_tree_model_get (model, iter, CONTACT_UID_COL, &uid, -1);
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
		gtk_tree_model_get (model, iter, CONTACT_NAME_COL, &name_string,
				    -1);
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
contacts_sort_treeview_cb (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b,
			   gpointer user_data)
{
	gchar *string1, *string2;
	gint returnval;

	gtk_tree_model_get (model, a, CONTACT_NAME_COL, &string1, -1);
	gtk_tree_model_get (model, b, CONTACT_NAME_COL, &string2, -1);
	if (!string1) string1 = g_new0 (gchar, 1);
	if (!string2) string2 = g_new0 (gchar, 1);
	returnval = strcasecmp ((const char *) string1,
				(const char *) string2);
	g_free (string1);
	g_free (string2);

	return returnval;
}

