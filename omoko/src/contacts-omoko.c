/*
 * contacts-gtk.c
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

#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <libmokoui/moko-paned-window.h>
#include <libmokoui/moko-tool-box.h>
#include "contacts-callbacks-ui.h"
#include "contacts-edit-pane.h"
#include "contacts-main.h"
#include "contacts-ui.h"

static void moko_filter_changed (GtkWidget *widget, gchar *text, ContactsData *data);
static GtkMenu *filter_menu;

void
create_main_window (ContactsData *contacts_data)
{
	GtkWidget *contacts_menu_menu;
	GtkWidget *contacts_import;
	GtkWidget *separatormenuitem1;
	GtkWidget *contacts_quit;
	GtkWidget *scrolledwindow2;
	GtkWidget *contacts_treeview;
	GtkWidget *scrolledwindow3;
	GtkWidget *viewport;
	GtkAccelGroup *accel_group;
	GtkWidget *moko_tool_box;
	ContactsUI *ui = contacts_data->ui;

	accel_group = gtk_accel_group_new ();

	ui->main_window = moko_paned_window_new ();
	gtk_window_set_title (GTK_WINDOW (ui->main_window), _("Contacts"));
	gtk_window_set_default_size (GTK_WINDOW (ui->main_window), -1, 300);
	gtk_window_set_icon_name (GTK_WINDOW (ui->main_window), "stock_contact");

	/* main menu */
	contacts_menu_menu = gtk_menu_new ();
	moko_paned_window_set_application_menu(MOKO_PANED_WINDOW (ui->main_window), GTK_MENU (contacts_menu_menu));

	ui->new_menuitem = gtk_image_menu_item_new_from_stock ("gtk-new", accel_group);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), ui->new_menuitem);

	ui->edit_menuitem = gtk_image_menu_item_new_from_stock ("gtk-open", accel_group);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), ui->edit_menuitem);
	gtk_widget_set_sensitive (ui->edit_menuitem, FALSE);

	ui->edit_groups = gtk_menu_item_new_with_mnemonic (_("_Groups"));
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), ui->edit_groups);

	ui->delete_menuitem = gtk_image_menu_item_new_from_stock ("gtk-delete", accel_group);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), ui->delete_menuitem);
	gtk_widget_set_sensitive (ui->delete_menuitem, FALSE);

	contacts_import = gtk_menu_item_new_with_mnemonic (_("_Import"));
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), contacts_import);

	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), separatormenuitem1);
	gtk_widget_set_sensitive (separatormenuitem1, FALSE);

	contacts_quit = gtk_image_menu_item_new_from_stock ("gtk-quit", accel_group);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), contacts_quit);
	/*** end menu creation ***/

	/*** filter ***/

	filter_menu = (GtkMenu*) gtk_menu_new ();
	moko_paned_window_set_filter_menu (MOKO_PANED_WINDOW (ui->main_window), GTK_MENU(filter_menu));

	/* contacts list vbox */
	scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
	moko_paned_window_set_upper_pane (MOKO_PANED_WINDOW (ui->main_window), scrolledwindow2);
	GTK_WIDGET_UNSET_FLAGS (scrolledwindow2, GTK_CAN_FOCUS);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_SHADOW_IN);

	contacts_treeview = gtk_tree_view_new ();
	gtk_container_add (GTK_CONTAINER (scrolledwindow2), contacts_treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (contacts_treeview), FALSE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (contacts_treeview), FALSE);

	/* tool box */
	moko_tool_box = moko_tool_box_new_with_search ();

	moko_paned_window_add_toolbox (MOKO_PANED_WINDOW (ui->main_window), MOKO_TOOL_BOX(moko_tool_box));
	ui->search_entry = (GtkWidget*)moko_tool_box_get_entry (MOKO_TOOL_BOX(moko_tool_box));

	/* FIXME? We seem to have to add these in reverse order at the moment */
	ui->delete_button = (GtkWidget *)moko_tool_box_add_action_button (MOKO_TOOL_BOX (moko_tool_box));
	moko_pixmap_button_set_center_stock (MOKO_PIXMAP_BUTTON (ui->delete_button), "gtk-delete");

	ui->edit_button = (GtkWidget *)moko_tool_box_add_action_button (MOKO_TOOL_BOX (moko_tool_box));
	moko_pixmap_button_set_center_stock (MOKO_PIXMAP_BUTTON (ui->edit_button), "gtk-edit");

	ui->new_button = (GtkWidget *)moko_tool_box_add_action_button (MOKO_TOOL_BOX (moko_tool_box));
	moko_pixmap_button_set_center_stock (MOKO_PIXMAP_BUTTON (ui->new_button), "gtk-new");


	/*** contacts display ***/

	/* note book for switching between view/edit mode */
	ui->main_notebook = gtk_notebook_new ();
	moko_paned_window_set_lower_pane (MOKO_PANED_WINDOW (ui->main_window), ui->main_notebook);
	GTK_WIDGET_UNSET_FLAGS (ui->main_notebook, GTK_CAN_FOCUS);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (ui->main_notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (ui->main_notebook), FALSE);

	/*** view mode ****/
	scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
	gtk_notebook_append_page (GTK_NOTEBOOK (ui->main_notebook), scrolledwindow3, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow3), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	viewport = gtk_viewport_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolledwindow3), viewport);

	ui->summary_vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_add (GTK_CONTAINER (viewport), ui->summary_vbox);
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
	GtkWidget *vbox4 = gtk_vbox_new (FALSE, 6);
	gtk_notebook_append_page (GTK_NOTEBOOK (ui->main_notebook), vbox4, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (vbox4), 0);

	GtkWidget *scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox4), scrolledwindow4, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	GtkWidget *viewport2 = gtk_viewport_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolledwindow4), viewport2);

	ui->edit_table = gtk_table_new (1, 2, FALSE);
	gtk_container_add (GTK_CONTAINER (viewport2), ui->edit_table);
	gtk_container_set_border_width (GTK_CONTAINER (ui->edit_table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (ui->edit_table), 6);

	/*** connect signals ***/
	g_signal_connect ((gpointer) ui->main_window, "destroy",
			G_CALLBACK (gtk_main_quit),
			NULL);
	g_signal_connect ((gpointer) contacts_quit, "activate",
			G_CALLBACK (gtk_main_quit),
			NULL);
	g_signal_connect_data ((gpointer) contacts_treeview, "key_press_event",
			G_CALLBACK (contacts_treeview_search_cb),
			GTK_OBJECT (ui->search_entry),
			NULL, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
	g_signal_connect ((gpointer) ui->search_entry, "changed",
			G_CALLBACK (contacts_search_changed_cb),
			contacts_data);
	g_signal_connect (G_OBJECT (ui->new_button), "clicked",
			  G_CALLBACK (contacts_new_cb), contacts_data);
	g_signal_connect (G_OBJECT (ui->new_menuitem), "activate",
			  G_CALLBACK (contacts_new_cb), contacts_data);
	g_signal_connect (G_OBJECT (ui->edit_button), "clicked",
			  G_CALLBACK (contacts_edit_cb), contacts_data);
	g_signal_connect (G_OBJECT (contacts_treeview), "row_activated",
			  G_CALLBACK (contacts_treeview_edit_cb), contacts_data);
	g_signal_connect (G_OBJECT (ui->delete_button), "clicked",
			  G_CALLBACK (contacts_delete_cb), contacts_data);
	g_signal_connect (G_OBJECT (ui->delete_menuitem), "activate",
			  G_CALLBACK (contacts_delete_cb), contacts_data);
	g_signal_connect (G_OBJECT (contacts_import), "activate",
			  G_CALLBACK (contacts_import_cb), contacts_data);
	g_signal_connect (G_OBJECT (moko_paned_window_get_menubox (MOKO_PANED_WINDOW (ui->main_window))),
			"filter_changed", G_CALLBACK (moko_filter_changed), contacts_data);
	g_signal_connect (G_OBJECT(moko_tool_box), "searchbox_invisible",
			  G_CALLBACK (contacts_disable_search_cb), contacts_data);
	g_signal_connect (G_OBJECT(moko_tool_box), "searchbox_visible",
			  G_CALLBACK (contacts_enable_search_cb), contacts_data);

	ui->contact_delete = NULL;//contact_delete;
	ui->contact_export = NULL;//contact_export;
	ui->contact_menu = NULL; //contact_menu;

	ui->contacts_import = contacts_import;
	ui->contacts_menu = NULL;//contacts_menu;
	ui->contacts_treeview = contacts_treeview;

	ui->copy_menuitem = NULL;//copy;
	ui->cut_menuitem = NULL;//cut;
	ui->edit_done_button = NULL;//edit_done_button;
	ui->edit_menu = NULL;//edit_menu;
	ui->main_menubar = NULL;//main_menubar;
	ui->paste_menuitem = NULL;// paste;

	ui->add_field_button = NULL;//add_field_button;
	ui->remove_field_button = NULL;//remove_field_button;

	ui->search_entry_hbox = NULL;//search_entry_hbox;
	ui->search_hbox = NULL; //search_hbox;
	ui->search_tab_hbox = NULL; //search_tab_hbox;

	ui->summary_hbuttonbox = NULL;//summary_hbuttonbox;


	/* temporary settings */
	GtkSettings *settings = gtk_settings_get_default ();
	g_object_set (settings,
			"gtk-button-images", FALSE,
			"gtk-menu-images", FALSE,
			"gtk-theme-name", "openmoko-standard",
			NULL);

	gtk_widget_show_all (ui->main_window);
}

void
create_chooser_dialog (ContactsData *data)
{
	GtkWidget *chooser_dialog;
	GtkWidget *dialog_vbox1;
	GtkWidget *vbox6;
	GtkWidget *chooser_label;
	GtkWidget *chooser_add_hbox;
	GtkWidget *chooser_entry;
	GtkWidget *add_type_button;
	GtkWidget *scrolledwindow5;
	GtkWidget *chooser_treeview;
	GtkWidget *dialog_action_area1;
	GtkWidget *chooser_cancel_button;
	GtkWidget *chooser_ok_button;

	chooser_dialog = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (chooser_dialog), _("Edit Types"));
	gtk_window_set_modal (GTK_WINDOW (chooser_dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (chooser_dialog), -1, 280);
	gtk_window_set_type_hint (GTK_WINDOW (chooser_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_dialog_set_has_separator (GTK_DIALOG (chooser_dialog), FALSE);

	dialog_vbox1 = GTK_DIALOG (chooser_dialog)->vbox;

	vbox6 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox6, TRUE, TRUE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox6), 6);

	chooser_label = gtk_label_new (_("<span><b>Make a choice:</b></span>"));
	gtk_box_pack_start (GTK_BOX (vbox6), chooser_label, FALSE, FALSE, 6);
	gtk_label_set_use_markup (GTK_LABEL (chooser_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (chooser_label), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (chooser_label), 6, 0);

	chooser_add_hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox6), chooser_add_hbox, FALSE, TRUE, 6);

	chooser_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (chooser_add_hbox), chooser_entry, TRUE, TRUE, 0);
	gtk_entry_set_activates_default (GTK_ENTRY (chooser_entry), TRUE);

	add_type_button = gtk_button_new_from_stock ("gtk-add");
	gtk_box_pack_start (GTK_BOX (chooser_add_hbox), add_type_button, FALSE, FALSE, 0);
	GTK_WIDGET_SET_FLAGS (add_type_button, GTK_CAN_DEFAULT);
	gtk_button_set_focus_on_click (GTK_BUTTON (add_type_button), FALSE);

	scrolledwindow5 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox6), scrolledwindow5, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow5), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow5), GTK_SHADOW_IN);

	chooser_treeview = gtk_tree_view_new ();
	gtk_container_add (GTK_CONTAINER (scrolledwindow5), chooser_treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (chooser_treeview), FALSE);

	dialog_action_area1 = GTK_DIALOG (chooser_dialog)->action_area;
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

	chooser_cancel_button = gtk_button_new_from_stock ("gtk-cancel");
	gtk_dialog_add_action_widget (GTK_DIALOG (chooser_dialog), chooser_cancel_button, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (chooser_cancel_button, GTK_CAN_DEFAULT);

	chooser_ok_button = gtk_button_new_from_stock ("gtk-ok");
	gtk_dialog_add_action_widget (GTK_DIALOG (chooser_dialog), chooser_ok_button, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (chooser_ok_button, GTK_CAN_DEFAULT);

	gtk_widget_grab_focus (chooser_entry);
	gtk_widget_grab_default (add_type_button);

	/* connect signals */
	g_signal_connect ((gpointer) add_type_button, "clicked",
			G_CALLBACK (contacts_chooser_add_cb),
			data);


	data->ui->chooser_add_hbox = chooser_add_hbox;
	data->ui->chooser_dialog = chooser_dialog;
	data->ui->chooser_entry = chooser_entry;
	data->ui->chooser_label = chooser_label;
	data->ui->chooser_treeview = chooser_treeview;

}

void
contacts_create_ui (ContactsData *data)
{
	create_main_window (data);
	create_chooser_dialog (data);
}

static void
moko_filter_changed (GtkWidget *widget, gchar *text, ContactsData *data)
{
	g_free (data->selected_group);
	data->selected_group = g_strdup (text);
	contacts_update_treeview (data);
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

	g_list_foreach (data->contacts_groups, (GFunc)add_menu_item,
			filter_menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (filter_menu),
			gtk_separator_menu_item_new ());
	gtk_menu_shell_append (GTK_MENU_SHELL (filter_menu),
			gtk_menu_item_new_with_label (_("Search Results")));
	gtk_menu_shell_append (GTK_MENU_SHELL (filter_menu),
			gtk_menu_item_new_with_label (_("All")));

	gtk_widget_show_all (GTK_WIDGET (filter_menu));

}
