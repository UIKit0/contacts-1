
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libebook/e-book.h>

#include "callbacks-ui.h"
#include "globals.h"
#include "defs.h"
#include "utils.h"

void
contacts_added (EBookView *book_view, const GList *contacts)
{
	GtkComboBox *groups_combobox;
	GtkListStore *model;
	GtkTreeModelFilter *filter;
	GList *c;

	/* Get TreeView model and combo box to add contacts/groups */
	filter = GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model 
				 	(GTK_TREE_VIEW (glade_xml_get_widget
						(xml, "contacts_treeview"))));
	model = GTK_LIST_STORE (gtk_tree_model_filter_get_model (filter));
	groups_combobox = GTK_COMBO_BOX (glade_xml_get_widget 
					 (xml, "groups_combobox"));

	/* Iterate over new contacts and add them to the list */
	for (c = (GList *)contacts; c; c = c->next) {
		GList *contact_groups;
		const gchar *name, *uid;
		EContact *contact = E_CONTACT (c->data);
		EContactListHash *hash;
		
		if (!E_IS_CONTACT (contact))
			continue;

		/* Add contact to list */
		hash = g_new (EContactListHash, 1);
		name = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
		uid = e_contact_get_const (contact, E_CONTACT_UID);
		gtk_list_store_insert_with_values (model, &hash->iter, 0,
						   NAME, name,
						   UID, uid,
						   -1);
		hash->contact = g_object_ref (contact);
		g_hash_table_insert (contacts_table, (gchar *)uid, hash);

		/* Check for groups and add them to group list */
		contact_groups =
		    e_contact_get (contact,
				   E_CONTACT_CATEGORY_LIST);
		if (contact_groups) {
			GList *group;
			for (group = contact_groups; group;
			     group = group->next) {
				if (!g_list_find_custom (contacts_groups, 
							 group->data,
							 (GCompareFunc) strcmp))
				{
					gtk_combo_box_append_text
					    (groups_combobox, group->data);
					contacts_groups = g_list_prepend 
							     (contacts_groups,
							      group->data);
				}
			}
			g_list_free (contact_groups);
		}
	}
	
	/* Update view */
	update_treeview ();
}

void
contacts_changed (EBookView *book_view, const GList *contacts)
{
	GList *c;
	EContact *current_contact = get_current_contact ();
	if (current_contact) g_object_ref (current_contact);

	/* Loop through changed contacts */	
	for (c = (GList *)contacts; c; c = c->next) {
		EContact *contact = E_CONTACT (c->data);
		EContactListHash *hash;
		const gchar *uid;

		/* Lookup if contact exists in internal list (it should) */
		uid = e_contact_get_const (contact, E_CONTACT_UID);
		hash = g_hash_table_lookup (contacts_table, uid);
		if (!hash) continue;

		/* TODO: There's some funniness going on here... */
		/* Replace contact */
/*		g_object_unref (hash->contact);*/
		hash->contact = g_object_ref (contact);
		g_hash_table_steal (contacts_table, uid);
		g_hash_table_insert (contacts_table, (gchar *)uid, hash);

		/* Update list with possibly new name */
		GtkListStore *model = GTK_LIST_STORE 
			(gtk_tree_model_filter_get_model 
			 (GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model 
			  (GTK_TREE_VIEW (glade_xml_get_widget
					(xml, "contacts_treeview"))))));
		gtk_list_store_set (model, &hash->iter,
			NAME,
			e_contact_get (contact, E_CONTACT_FULL_NAME),
			-1);

		/* If contact is currently selected, update display */
		if (current_contact) {
			if (strcmp (e_contact_get_const
					(contact, E_CONTACT_UID),
				    e_contact_get_const
					(current_contact, E_CONTACT_UID)) == 0)
				display_contact_summary (contact);
		}
	}
	
	if (current_contact) g_object_unref (current_contact);

	/* Update view */
	update_treeview ();
}

void
contacts_removed (EBookView *book_view, const GList *ids)
{
	GList *i;
	
	for (i = (GList *)ids; i; i = i->next) {
		const gchar *uid = (const gchar *)i->data;
		g_hash_table_remove (contacts_table, uid);
	}

	/* Update view */
	update_treeview ();
}

