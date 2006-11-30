#include "config.h"
#include <gtk/gtk.h>
#include "contacts-callbacks-ui.h"
#include "contacts-ui.h"
#include FRONTEND_HEADER

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
contacts_setup_ui (ContactsData *data)
{
	GtkComboBox *groups_combobox;
	GtkTreeView *contacts_treeview;
	GtkTreeSelection *selection;
	GtkListStore *model;
	GtkTreeModelFilter *filter;
	GtkCellRenderer *renderer;
	GtkSizeGroup *size_group;

	/* these are defined in the frontend header */
	contacts_create_ui (data);

	/* Add the column to the GtkTreeView */
	contacts_treeview = GTK_TREE_VIEW (data->ui->contacts_treeview);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, "ypad", 0, NULL);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW
						     (contacts_treeview),
						     -1, NULL, renderer,
						     "text", CONTACT_NAME_COL,
						     NULL);
	/* Create model and groups/search filter for contacts list */
	model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	filter =
	    GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new
				   (GTK_TREE_MODEL (model), NULL));
	gtk_tree_model_filter_set_visible_func (filter,
						(GtkTreeModelFilterVisibleFunc)
						 contacts_is_row_visible_cb,
						data->contacts_table,
						NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (contacts_treeview),
				 GTK_TREE_MODEL (filter));
	/* Alphabetise the list */
	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (model),
						 contacts_sort_treeview_cb,
						 NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
				      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
				      GTK_SORT_ASCENDING);
	g_object_unref (model);


	/* Connect signal for selection changed event */
	selection = gtk_tree_view_get_selection (contacts_treeview);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (contacts_selection_cb), data);

	/* Enable multiple select (for delete) */
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	/* Create model/view for groups/type chooser list */
	contacts_treeview = GTK_TREE_VIEW (data->ui->chooser_treeview);
	model = gtk_list_store_new (2, G_TYPE_BOOLEAN, G_TYPE_STRING);
	renderer = gtk_cell_renderer_toggle_new ();
	g_object_set (renderer, "activatable", TRUE, NULL);
	g_signal_connect (renderer, "toggled",
			  (GCallback) contacts_chooser_toggle_cb, model);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW
						     (contacts_treeview), -1,
						     NULL, renderer, "active",
						     CHOOSER_TICK_COL, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW
						     (contacts_treeview), -1,
						     NULL, renderer, "text",
						     CHOOSER_NAME_COL, NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (contacts_treeview),
				 GTK_TREE_MODEL (model));
	g_object_unref (model);

	/* Select 'All' in the groups combobox */
	groups_combobox = GTK_COMBO_BOX (data->ui->groups_combobox);
	gtk_combo_box_set_active (groups_combobox, 0);

	/* Set transient parent for chooser */
	gtk_window_set_transient_for (
		GTK_WINDOW (data->ui->chooser_dialog),
		GTK_WINDOW (data->ui->main_window));

	/* Remove selectable label from focus chain */
	contacts_remove_labels_from_focus_chain (GTK_CONTAINER (data->ui->preview_header_hbox));

	/* Set up size group for bottom row of buttons and search */
	size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
	gtk_size_group_add_widget (size_group, data->ui->search_hbox);
	gtk_size_group_add_widget (size_group, data->ui->summary_hbuttonbox);
	g_object_unref (size_group);
}
