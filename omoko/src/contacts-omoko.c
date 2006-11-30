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

void
create_main_window (ContactsData *contacts_data)
{
	GtkWidget *main_window;
	GtkWidget *main_menubar;
	//GtkWidget *contacts_menu;
	GtkWidget *contacts_menu_menu;
	GtkWidget *new_menuitem;
	GtkWidget *edit_menuitem;
	GtkWidget *delete_menuitem;
	GtkWidget *contacts_import;
	GtkWidget *separatormenuitem1;
	GtkWidget *contacts_quit;
	//GtkWidget *contact_menu;
	//GtkWidget *contact_delete;
	GtkWidget *edit_groups;
	//GtkWidget *contact_export;
	GtkWidget *edit_menu;
	/*GtkWidget *cut;
	GtkWidget *copy;
	GtkWidget *paste;*/
	GtkWidget *main_notebook;
	GtkWidget *scrolledwindow2;
	GtkWidget *contacts_treeview;
	GtkWidget *search_entry;
	GtkWidget *summary_vbox;
	GtkWidget *preview_header_hbox;
	GtkWidget *summary_name_label;
	GtkWidget *photo_image;
	GtkWidget *scrolledwindow3;
	GtkWidget *viewport;
	GtkWidget *summary_table;
	GtkWidget *summary_hbuttonbox;
	GtkWidget *new_button;
	GtkWidget *edit_button;
	GtkWidget *delete_button;
	GtkWidget *edit_table;
	GtkWidget *add_field_button;
	GtkWidget *remove_field_button;
	GtkWidget *edit_done_button;
	GtkAccelGroup *accel_group;
	ContactsUI *ui = contacts_data->ui;

	accel_group = gtk_accel_group_new ();

	main_window = moko_paned_window_new ();
	gtk_window_set_title (GTK_WINDOW (main_window), _("Contacts"));
	gtk_window_set_default_size (GTK_WINDOW (main_window), -1, 300);
	gtk_window_set_icon_name (GTK_WINDOW (main_window), "stock_contact");

	contacts_menu_menu = gtk_menu_new ();
	moko_paned_window_set_application_menu(MOKO_PANED_WINDOW (main_window), GTK_MENU (contacts_menu_menu));

	new_menuitem = gtk_image_menu_item_new_from_stock ("gtk-new", accel_group);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), new_menuitem);

	edit_menuitem = gtk_image_menu_item_new_from_stock ("gtk-open", accel_group);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), edit_menuitem);
	gtk_widget_set_sensitive (edit_menuitem, FALSE);

	delete_menuitem = gtk_image_menu_item_new_from_stock ("gtk-delete", accel_group);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), delete_menuitem);
	gtk_widget_set_sensitive (delete_menuitem, FALSE);

	contacts_import = gtk_menu_item_new_with_mnemonic (_("_Import"));
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), contacts_import);

	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), separatormenuitem1);
	gtk_widget_set_sensitive (separatormenuitem1, FALSE);

	contacts_quit = gtk_image_menu_item_new_from_stock ("gtk-quit", accel_group);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), contacts_quit);
	/*** end menu creation ***/

	/*** filter ***/

	GtkWidget *filter_menu = gtk_menu_new ();
	GtkWidget *filter_all = gtk_menu_item_new_with_label (_("All"));
	gtk_container_add (GTK_CONTAINER (filter_menu), filter_all);
	moko_paned_window_set_filter_menu (MOKO_PANED_WINDOW (main_window), GTK_MENU(filter_menu));

	/* contacts list vbox */
	scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
	moko_paned_window_set_upper_pane (MOKO_PANED_WINDOW (main_window), scrolledwindow2);
	GTK_WIDGET_UNSET_FLAGS (scrolledwindow2, GTK_CAN_FOCUS);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_SHADOW_IN);

	contacts_treeview = gtk_tree_view_new ();
	gtk_container_add (GTK_CONTAINER (scrolledwindow2), contacts_treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (contacts_treeview), FALSE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (contacts_treeview), FALSE);

	/* tool box */
	GtkWidget *moko_tool_box = moko_tool_box_new_with_search ();
	GtkHBox *moko_button_box = moko_tool_box_get_button_box (MOKO_TOOL_BOX (moko_tool_box));

	moko_paned_window_add_toolbox (MOKO_PANED_WINDOW (main_window), MOKO_TOOL_BOX(moko_tool_box));
	search_entry = (GtkWidget*)moko_tool_box_get_entry (MOKO_TOOL_BOX(moko_tool_box));

	new_button = gtk_button_new_from_stock ("gtk-new");
	gtk_container_add (GTK_CONTAINER (moko_button_box), new_button);
	GTK_WIDGET_SET_FLAGS (new_button, GTK_CAN_DEFAULT);

	edit_button = gtk_button_new_from_stock ("gtk-edit");
	gtk_container_add (GTK_CONTAINER (moko_button_box), edit_button);
	gtk_widget_set_sensitive (edit_button, FALSE);
	GTK_WIDGET_SET_FLAGS (edit_button, GTK_CAN_DEFAULT);

	delete_button = gtk_button_new_from_stock ("gtk-delete");
	gtk_container_add (GTK_CONTAINER (moko_button_box), delete_button);
	gtk_widget_set_sensitive (delete_button, FALSE);
	GTK_WIDGET_SET_FLAGS (delete_button, GTK_CAN_DEFAULT);
	gtk_button_set_focus_on_click (GTK_BUTTON (delete_button), FALSE);


	/*** contacts display ***/

	/* note book for switching between view/edit mode */
	main_notebook = gtk_notebook_new ();
	moko_paned_window_set_lower_pane (MOKO_PANED_WINDOW (main_window), main_notebook);
	GTK_WIDGET_UNSET_FLAGS (main_notebook, GTK_CAN_FOCUS);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (main_notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (main_notebook), FALSE);

	/*** view mode ****/
	scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
	gtk_notebook_append_page (GTK_NOTEBOOK (main_notebook), scrolledwindow3, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow3), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	viewport = gtk_viewport_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolledwindow3), viewport);

	summary_vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_add (GTK_CONTAINER (viewport), summary_vbox);
	g_object_set (summary_vbox, "border-width", 12, NULL);

	preview_header_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (summary_vbox), preview_header_hbox, FALSE, TRUE, 0);

	photo_image = gtk_image_new ();
	gtk_box_pack_start (GTK_BOX (preview_header_hbox), photo_image, FALSE, TRUE, 6);
	gtk_misc_set_padding (GTK_MISC (photo_image), 1, 0);

	summary_name_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (preview_header_hbox), summary_name_label, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (summary_name_label, GTK_CAN_FOCUS);
	gtk_label_set_use_markup (GTK_LABEL (summary_name_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (summary_name_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (summary_name_label), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (summary_name_label), 6, 0);
	gtk_label_set_ellipsize (GTK_LABEL (summary_name_label), PANGO_ELLIPSIZE_END);

	gtk_box_pack_start (GTK_BOX (summary_vbox), gtk_hseparator_new (), FALSE, TRUE, 0);

	summary_table = gtk_table_new (1, 2, FALSE);
	gtk_box_pack_start (GTK_BOX (summary_vbox), summary_table, FALSE, TRUE, 6);
	gtk_table_set_row_spacings (GTK_TABLE (summary_table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (summary_table), 6);

	/*** edit mode ***/
	GtkWidget *vbox4 = gtk_vbox_new (FALSE, 6);
	gtk_notebook_append_page (GTK_NOTEBOOK (main_notebook), vbox4, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (vbox4), 6);

	GtkWidget *scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox4), scrolledwindow4, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	GtkWidget *viewport2 = gtk_viewport_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolledwindow4), viewport2);

	edit_table = gtk_table_new (1, 2, FALSE);
	gtk_container_add (GTK_CONTAINER (viewport2), edit_table);
	gtk_container_set_border_width (GTK_CONTAINER (edit_table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (edit_table), 6);

	/*
	GtkWidget *hbuttonbox2 = gtk_hbutton_box_new ();
	gtk_box_pack_start (GTK_BOX (vbox4), hbuttonbox2, FALSE, TRUE, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox2), GTK_BUTTONBOX_END);

	edit_done_button = gtk_button_new_from_stock ("gtk-close");
	gtk_container_add (GTK_CONTAINER (hbuttonbox2), edit_done_button);
	GTK_WIDGET_SET_FLAGS (edit_done_button, GTK_CAN_DEFAULT);
	*/

	/*** connect signals ***/
	g_signal_connect ((gpointer) main_window, "destroy",
			G_CALLBACK (gtk_main_quit),
			NULL);
	
	g_signal_connect_swapped ((gpointer) main_window, "set_focus",
			G_CALLBACK (contacts_edit_set_focus_cb),
			GTK_OBJECT (remove_field_button));
			
	g_signal_connect ((gpointer) contacts_quit, "activate",
			G_CALLBACK (gtk_main_quit),
			NULL);

	g_signal_connect_data ((gpointer) contacts_treeview, "key_press_event",
			G_CALLBACK (contacts_treeview_search_cb),
			GTK_OBJECT (search_entry),
			NULL, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
	g_signal_connect ((gpointer) search_entry, "changed",
			G_CALLBACK (contacts_search_changed_cb),
			contacts_data);
/*
	g_signal_connect ((gpointer) remove_field_button, "clicked",
			G_CALLBACK (contacts_remove_field_cb),
			NULL);
			*/
	g_signal_connect (G_OBJECT (new_button), "clicked",
			  G_CALLBACK (contacts_new_cb), contacts_data);
	g_signal_connect (G_OBJECT (new_menuitem), "activate",
			  G_CALLBACK (contacts_new_cb), contacts_data);
	g_signal_connect (G_OBJECT (edit_button), "clicked",
			  G_CALLBACK (contacts_edit_cb), contacts_data);
	g_signal_connect (G_OBJECT (contacts_treeview), "row_activated",
			  G_CALLBACK (contacts_treeview_edit_cb), contacts_data);
	/*g_signal_connect (G_OBJECT (edit_menuitem), "activate",
			  G_CALLBACK (contacts_edit_cb), contacts_data);*/
	g_signal_connect (G_OBJECT (delete_button), "clicked",
			  G_CALLBACK (contacts_delete_cb), contacts_data);
	g_signal_connect (G_OBJECT (delete_menuitem), "activate",
			  G_CALLBACK (contacts_delete_cb), contacts_data);
	g_signal_connect (G_OBJECT (contacts_import), "activate",
			  G_CALLBACK (contacts_import_cb), contacts_data);
/*	g_signal_connect (G_OBJECT (edit_menu), "activate",
			  G_CALLBACK (contacts_edit_menu_activate_cb), contacts_data);
*/
	ui->contact_delete = NULL;//contact_delete;
	ui->contact_export = NULL;//contact_export;
	ui->contact_menu = NULL; //contact_menu;

	ui->contacts_import = contacts_import;
	ui->contacts_menu = NULL;//contacts_menu;
	ui->contacts_treeview = contacts_treeview;

	ui->new_menuitem = new_menuitem;
	ui->copy_menuitem = NULL;//copy;
	ui->cut_menuitem = NULL;//cut;
	ui->delete_menuitem = delete_menuitem;
	ui->delete_button = delete_button;
	ui->edit_menuitem = edit_menuitem;
	ui->edit_button = edit_button;
	ui->edit_done_button = edit_done_button;
	ui->edit_groups = edit_groups;
	ui->edit_menu = edit_menu;
	ui->edit_table = edit_table;
	ui->main_menubar = main_menubar;
	ui->main_notebook = main_notebook;
	ui->main_window = main_window;
	ui->new_button = new_button;
	ui->paste_menuitem = NULL;// paste;
	ui->photo_image = photo_image;
	ui->preview_header_hbox = preview_header_hbox;

	ui->add_field_button = add_field_button;
	ui->remove_field_button = remove_field_button;

	ui->search_entry = search_entry;
	ui->search_entry_hbox = NULL;//search_entry_hbox;
	ui->search_hbox = NULL; //search_hbox;
	ui->search_tab_hbox = NULL; //search_tab_hbox;

	ui->summary_hbuttonbox = summary_hbuttonbox;
	ui->summary_name_label = summary_name_label;
	ui->summary_table = summary_table;
	ui->summary_vbox = summary_vbox;


	GtkSettings *settings = gtk_settings_get_default ();
	g_object_set (settings,
			"gtk-button-images", FALSE,
			"gtk-menu-images", FALSE,
			"gtk-theme-name", "openmoko-standard",
			NULL);

	gtk_widget_show_all (main_window);
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

	//gtk_widget_show_all (chooser_dialog);

}

void
contacts_create_ui (ContactsData *data)
{
	create_main_window (data);
	create_chooser_dialog (data);
}

void
contacts_update_groups_list (ContactsData *data)
{
}
