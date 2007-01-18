/*
 * contacts-omoko.c
 * This file is part of Contacts
 *
 * Copyright (C) 2006 - OpenedHand Ltd
 *
 * Contacts is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Contacts is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Contacts; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include "contacts-callbacks-ui.h"
#include "contacts-edit-pane.h"
#include "contacts-main.h"
#include "contacts-ui.h"
#include "contacts-omoko.h"
#include "contacts-groups-editor.h"



static void moko_filter_changed (GtkWidget *widget, gchar *text, ContactsData *data);

/* these are specific to the omoko frontend */
static GtkMenu *filter_menu;


GtkWidget *
create_contacts_list (ContactsData *data)
{
	MokoNavigationList *moko_navigation_list = moko_navigation_list_new ();
	GtkWidget *treeview = GTK_WIDGET (moko_navigation_list_get_tree_view (moko_navigation_list));

	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
				 GTK_TREE_MODEL (data->contacts_filter));


	/* add columns to treeview */
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* name column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,
							"text", CONTACT_NAME_COL, NULL);
	gtk_tree_view_column_set_sort_column_id (column, CONTACT_NAME_COL);
	moko_navigation_list_append_column (moko_navigation_list, column);

	/* mobile column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Cell Phone"), renderer,
							"text", CONTACT_CELLPHONE_COL, NULL);
	gtk_tree_view_column_set_sort_column_id (column, CONTACT_CELLPHONE_COL);
	moko_navigation_list_append_column (moko_navigation_list, column);

	return GTK_WIDGET (moko_navigation_list);
}

void
create_main_window (ContactsData *contacts_data)
{
	GtkWidget *contacts_menu_menu, *scrolledwindow3, *widget;
	GtkAccelGroup *accel_group;
	GtkWidget *moko_tool_box;
	ContactsUI *ui = contacts_data->ui;

	accel_group = gtk_accel_group_new ();

	//? MokoApplication* app = MOKO_APPLICATION(moko_application_get_instance());

	g_set_application_name ("Phone Book");

	ui->main_window = moko_paned_window_new ();
	gtk_window_set_title (GTK_WINDOW (ui->main_window), _("Contacts"));
	gtk_window_set_default_size (GTK_WINDOW (ui->main_window), -1, 300);
	gtk_window_set_icon_name (GTK_WINDOW (ui->main_window), "stock_contact");

	/* main menu */
	contacts_menu_menu = gtk_menu_new ();
	moko_paned_window_set_application_menu(MOKO_PANED_WINDOW (ui->main_window), GTK_MENU (contacts_menu_menu));

	widget = gtk_menu_item_new_with_label ("New Contact");
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), GTK_WIDGET (widget));
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (contacts_new_cb), contacts_data);

	widget = gtk_menu_item_new_with_label ("New Group");
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), GTK_WIDGET (widget));

	widget = gtk_menu_item_new_with_label ("Import");
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), GTK_WIDGET (widget));
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (contacts_import_cb), contacts_data);

	widget = gtk_menu_item_new_with_label ("Export");
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), GTK_WIDGET (widget));
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (contacts_export_cb), contacts_data);

	ui->delete_menuitem = gtk_menu_item_new_with_label ("Delete Contact");
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), GTK_WIDGET (ui->delete_menuitem));
	g_signal_connect (G_OBJECT (ui->delete_menuitem), "activate",
			  G_CALLBACK (contacts_delete_cb), contacts_data);

	widget = gtk_menu_item_new_with_label ("Exit");
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), GTK_WIDGET (widget));
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (gtk_main_quit), NULL);


	/*** end menu creation ***/

	/*** filter menu ***/

	filter_menu = (GtkMenu*) gtk_menu_new ();
	moko_paned_window_set_filter_menu (MOKO_PANED_WINDOW (ui->main_window), GTK_MENU(filter_menu));

	/*** contacts list ***/

	GtkWidget *moko_navigation_list = create_contacts_list (contacts_data);
	g_object_unref (contacts_data->contacts_liststore);

	moko_paned_window_set_upper_pane (MOKO_PANED_WINDOW (ui->main_window), GTK_WIDGET (moko_navigation_list));
	ui->contacts_treeview = GTK_WIDGET (moko_navigation_list_get_tree_view (MOKO_NAVIGATION_LIST (moko_navigation_list)));

	/* Connect signal for selection changed event */
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->contacts_treeview));
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (contacts_selection_cb), contacts_data);

	/* Enable multiple select (for delete) */
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);


	/* tool box */
	moko_tool_box = moko_tool_box_new_with_search ();

	moko_paned_window_add_toolbox (MOKO_PANED_WINDOW (ui->main_window), MOKO_TOOL_BOX(moko_tool_box));
	ui->search_entry = (GtkWidget*)moko_tool_box_get_entry (MOKO_TOOL_BOX(moko_tool_box));

	/* FIXME? We seem to have to add these in reverse order at the moment */

	/* groups button */
	widget = GTK_WIDGET (moko_tool_box_add_action_button (MOKO_TOOL_BOX (moko_tool_box)));
	moko_pixmap_button_set_action_btn_upper_stock (MOKO_PIXMAP_BUTTON (widget),
						       "openmoko-action-button-group-icon");
	moko_pixmap_button_set_action_btn_lower_label (MOKO_PIXMAP_BUTTON (widget),
						       "Group");
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (contacts_groups_pane_show), contacts_data);

	/* history button */
	widget = GTK_WIDGET (moko_tool_box_add_action_button (MOKO_TOOL_BOX (moko_tool_box)));
	moko_pixmap_button_set_action_btn_upper_stock (MOKO_PIXMAP_BUTTON (widget),
						       "openmoko-filter-menu-icon");
	moko_pixmap_button_set_action_btn_lower_label (MOKO_PIXMAP_BUTTON (widget),
						       "History");

	/* edit button */
	widget = GTK_WIDGET (moko_tool_box_add_action_button (MOKO_TOOL_BOX (moko_tool_box)));
	moko_pixmap_button_set_action_btn_upper_stock (MOKO_PIXMAP_BUTTON (widget),
						       "openmoko-filter-menu-icon");
	moko_pixmap_button_set_action_btn_lower_label (MOKO_PIXMAP_BUTTON (widget),
						       "Edit");
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (contacts_edit_cb), contacts_data);

	/* view button */
	widget = GTK_WIDGET (moko_tool_box_add_action_button (MOKO_TOOL_BOX (moko_tool_box)));
	moko_pixmap_button_set_action_btn_upper_stock (MOKO_PIXMAP_BUTTON (widget),
						       "openmoko-filter-menu-icon");
	moko_pixmap_button_set_action_btn_lower_label (MOKO_PIXMAP_BUTTON (widget),
						       "View");
	g_signal_connect (G_OBJECT (widget), "clicked",
			  G_CALLBACK (contacts_view_cb), contacts_data);


	/*** contacts display ***/

	/* note book for switching between view/edit mode */
	ui->main_notebook = gtk_notebook_new ();
	scrolledwindow3 = (GtkWidget*) moko_details_window_new();
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindow3), GTK_WIDGET (ui->main_notebook));
	moko_paned_window_set_lower_pane (MOKO_PANED_WINDOW (ui->main_window), GTK_WIDGET(moko_details_window_put_in_box (MOKO_DETAILS_WINDOW (scrolledwindow3))));
	GTK_WIDGET_UNSET_FLAGS (ui->main_notebook, GTK_CAN_FOCUS);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (ui->main_notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (ui->main_notebook), FALSE);

	/*** view mode ****/
	ui->summary_vbox = gtk_vbox_new (FALSE, 6);
	gtk_notebook_append_page (GTK_NOTEBOOK (ui->main_notebook), ui->summary_vbox, NULL);
	g_object_set (ui->summary_vbox, "border-width", 12, NULL);

	ui->preview_header_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (ui->summary_vbox), ui->preview_header_hbox, FALSE, TRUE, 0);

	ui->photo_image = gtk_image_new ();
	gtk_box_pack_start (GTK_BOX (ui->preview_header_hbox), ui->photo_image, FALSE, TRUE, 6);
	gtk_misc_set_padding (GTK_MISC (ui->photo_image), 1, 0);

	ui->summary_name_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (ui->preview_header_hbox), ui->summary_name_label, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (ui->summary_name_label, GTK_CAN_FOCUS);
	gtk_label_set_use_markup (GTK_LABEL (ui->summary_name_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (ui->summary_name_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (ui->summary_name_label), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (ui->summary_name_label), 6, 0);
	gtk_label_set_ellipsize (GTK_LABEL (ui->summary_name_label), PANGO_ELLIPSIZE_END);

	gtk_box_pack_start (GTK_BOX (ui->summary_vbox), gtk_hseparator_new (), FALSE, TRUE, 0);

	ui->summary_table = gtk_table_new (1, 2, FALSE);
	gtk_box_pack_start (GTK_BOX (ui->summary_vbox), ui->summary_table, FALSE, TRUE, 6);
	gtk_table_set_row_spacings (GTK_TABLE (ui->summary_table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (ui->summary_table), 6);

	/*** edit mode ***/
	ui->edit_table = gtk_table_new (1, 2, FALSE);
	gtk_notebook_append_page (GTK_NOTEBOOK (ui->main_notebook), ui->edit_table, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (ui->edit_table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (ui->edit_table), 6);

	/*** groups mode ***/
	ui->groups_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (ui->groups_vbox), 12);
	gtk_notebook_append_page (GTK_NOTEBOOK (ui->main_notebook), ui->groups_vbox, NULL);

	/*** connect signals ***/
	g_signal_connect ((gpointer) ui->main_window, "destroy",
			G_CALLBACK (gtk_main_quit),
			NULL);
	g_signal_connect_data ((gpointer) ui->contacts_treeview, "key_press_event",
			G_CALLBACK (contacts_treeview_search_cb),
			GTK_OBJECT (ui->search_entry),
			NULL, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
	g_signal_connect ((gpointer) ui->search_entry, "changed",
			G_CALLBACK (contacts_search_changed_cb),
			contacts_data);
	g_signal_connect (G_OBJECT (ui->contacts_treeview), "row_activated",
			  G_CALLBACK (contacts_treeview_edit_cb), contacts_data);

	g_signal_connect (G_OBJECT (moko_paned_window_get_menubox (MOKO_PANED_WINDOW (ui->main_window))),
			"filter_changed", G_CALLBACK (moko_filter_changed), contacts_data);
	g_signal_connect (G_OBJECT(moko_tool_box), "searchbox_invisible",
			  G_CALLBACK (contacts_disable_search_cb), contacts_data);
	g_signal_connect (G_OBJECT(moko_tool_box), "searchbox_visible",
			  G_CALLBACK (contacts_enable_search_cb), contacts_data);

	/* temporary settings */
	GtkSettings *settings = gtk_settings_get_default ();
	g_object_set (settings,
			"gtk-theme-name", "openmoko-standard",
			NULL);


	gtk_widget_show_all (ui->main_window);
}

static void
moko_filter_changed (GtkWidget *widget, gchar *text, ContactsData *data)
{
	g_free (data->selected_group);
	data->selected_group = g_strdup (text);
	contacts_update_treeview (data);
}

void
contacts_ui_create (ContactsData *data)
{
	create_main_window (data);
	//create_chooser_dialog (data);
}

void
contacts_ui_update_groups_list (ContactsData *data)
{
	void remove_menu_item (GtkWidget *menu_item, GtkWidget *menu)
	{
		gtk_container_remove (GTK_CONTAINER (menu), menu_item);
	}

	void add_menu_item (gchar *group, GtkMenu *menu)
	{
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_menu_item_new_with_label (group));
	}

	/* update filter menu */
	gtk_container_foreach (GTK_CONTAINER (filter_menu), (GtkCallback)remove_menu_item, filter_menu);

	gtk_menu_shell_append (GTK_MENU_SHELL (filter_menu),
			gtk_menu_item_new_with_label (_("All")));
	gtk_menu_shell_append (GTK_MENU_SHELL (filter_menu),
			gtk_separator_menu_item_new ());
	g_list_foreach (data->contacts_groups, (GFunc)add_menu_item,
			filter_menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (filter_menu),
			gtk_separator_menu_item_new ());
	gtk_menu_shell_append (GTK_MENU_SHELL (filter_menu),
			gtk_menu_item_new_with_label (_("Search Results")));

	gtk_widget_show_all (GTK_WIDGET (filter_menu));

}

