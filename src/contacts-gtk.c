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
#include "contacts-callbacks-ui.h"
#include "contacts-edit-pane.h"
#include "contacts-main.h"

void
create_main_window (ContactsData *contacts_data)
{
	GtkWidget *main_window;
	GtkWidget *vbox7;
	GtkWidget *main_menubar;
	GtkWidget *contacts_menu;
	GtkWidget *contacts_menu_menu;
	GtkWidget *new;
	GtkWidget *edit;
	GtkWidget *delete_menuitem;
	GtkWidget *contacts_import;
	GtkWidget *separatormenuitem1;
	GtkWidget *contacts_quit;
	GtkWidget *contact_menu;
	GtkWidget *contact_menu_menu;
	GtkWidget *contact_delete;
	GtkWidget *edit_groups;
	GtkWidget *contact_export;
	GtkWidget *separator1;
	GtkWidget *contact_quit;
	GtkWidget *edit_menu;
	GtkWidget *menuitem5_menu;
	GtkWidget *cut;
	GtkWidget *copy;
	GtkWidget *paste;
	GtkWidget *help_menu;
	GtkWidget *menuitem7_menu;
	GtkWidget *about1;
	GtkWidget *main_notebook;
	GtkWidget *main_hpane;
	GtkWidget *contacts_vbox;
	GtkWidget *groups_combobox;
	GtkWidget *scrolledwindow2;
	GtkWidget *contacts_treeview;
	GtkWidget *search_hbox;
	GtkWidget *search_entry_hbox;
	GtkWidget *search_label;
	GtkWidget *search_entry;
	GtkWidget *search_tab_hbox;
	GtkWidget *symbols_radiobutton;
	GSList *symbols_radiobutton_group = NULL;
	GtkWidget *atog_radiobutton;
	GtkWidget *hton_radiobutton;
	GtkWidget *otou_radiobutton;
	GtkWidget *vtoz_radiobutton;
	GtkWidget *vbox3;
	GtkWidget *summary_vbox;
	GtkWidget *preview_header_hbox;
	GtkWidget *summary_name_label;
	GtkWidget *photo_image;
	GtkWidget *scrolledwindow3;
	GtkWidget *viewport1;
	GtkWidget *summary_table;
	GtkWidget *summary_hbuttonbox;
	GtkWidget *new_button;
	GtkWidget *edit_button;
	GtkWidget *delete_button;
	GtkWidget *label1;
	GtkWidget *vbox4;
	GtkWidget *scrolledwindow4;
	GtkWidget *viewport2;
	GtkWidget *edit_table;
	GtkWidget *hbuttonbox2;
	GtkWidget *add_field_button;
	GtkWidget *remove_field_button;
	GtkWidget *edit_done_button;
	GtkWidget *label2;
	GtkWidget *empty_notebook_page;
	GtkWidget *label3;
	GtkWidget *label4;
	GtkAccelGroup *accel_group;
	ContactsUI *ui = contacts_data->ui;

	accel_group = gtk_accel_group_new ();

	main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (main_window), _("Contacts"));
	gtk_window_set_default_size (GTK_WINDOW (main_window), -1, 300);
	gtk_window_set_icon_name (GTK_WINDOW (main_window), "stock_contact");

	vbox7 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox7);
	gtk_container_add (GTK_CONTAINER (main_window), vbox7);

	main_menubar = gtk_menu_bar_new ();
	gtk_widget_show (main_menubar);
	gtk_box_pack_start (GTK_BOX (vbox7), main_menubar, FALSE, FALSE, 0);

	contacts_menu = gtk_menu_item_new_with_mnemonic (_("_Contacts"));
	gtk_widget_show (contacts_menu);
	gtk_container_add (GTK_CONTAINER (main_menubar), contacts_menu);

	contacts_menu_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (contacts_menu), contacts_menu_menu);

	new = gtk_image_menu_item_new_from_stock ("gtk-new", accel_group);
	gtk_widget_show (new);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), new);

	edit = gtk_image_menu_item_new_from_stock ("gtk-open", accel_group);
	gtk_widget_show (edit);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), edit);
	gtk_widget_set_sensitive (edit, FALSE);

	delete_menuitem = gtk_image_menu_item_new_from_stock ("gtk-delete", accel_group);
	gtk_widget_show (delete_menuitem);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), delete_menuitem);
	gtk_widget_set_sensitive (delete_menuitem, FALSE);

	contacts_import = gtk_menu_item_new_with_mnemonic (_("_Import"));
	gtk_widget_show (contacts_import);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), contacts_import);

	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem1);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), separatormenuitem1);
	gtk_widget_set_sensitive (separatormenuitem1, FALSE);

	contacts_quit = gtk_image_menu_item_new_from_stock ("gtk-quit", accel_group);
	gtk_widget_show (contacts_quit);
	gtk_container_add (GTK_CONTAINER (contacts_menu_menu), contacts_quit);

	contact_menu = gtk_menu_item_new_with_mnemonic (_("_Contact"));
	gtk_container_add (GTK_CONTAINER (main_menubar), contact_menu);

	contact_menu_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (contact_menu), contact_menu_menu);

	contact_delete = gtk_image_menu_item_new_from_stock ("gtk-delete", accel_group);
	gtk_widget_show (contact_delete);
	gtk_container_add (GTK_CONTAINER (contact_menu_menu), contact_delete);

	edit_groups = gtk_menu_item_new_with_mnemonic (_("_Groups"));
	gtk_widget_show (edit_groups);
	gtk_container_add (GTK_CONTAINER (contact_menu_menu), edit_groups);

	contact_export = gtk_menu_item_new_with_mnemonic (_("_Export"));
	gtk_widget_show (contact_export);
	gtk_container_add (GTK_CONTAINER (contact_menu_menu), contact_export);

	separator1 = gtk_separator_menu_item_new ();
	gtk_widget_show (separator1);
	gtk_container_add (GTK_CONTAINER (contact_menu_menu), separator1);
	gtk_widget_set_sensitive (separator1, FALSE);

	contact_quit = gtk_image_menu_item_new_from_stock ("gtk-quit", accel_group);
	gtk_widget_show (contact_quit);
	gtk_container_add (GTK_CONTAINER (contact_menu_menu), contact_quit);

	edit_menu = gtk_menu_item_new_with_mnemonic (_("_Edit"));
	gtk_widget_show (edit_menu);
	gtk_container_add (GTK_CONTAINER (main_menubar), edit_menu);

	menuitem5_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (edit_menu), menuitem5_menu);

	cut = gtk_image_menu_item_new_from_stock ("gtk-cut", accel_group);
	gtk_widget_show (cut);
	gtk_container_add (GTK_CONTAINER (menuitem5_menu), cut);

	copy = gtk_image_menu_item_new_from_stock ("gtk-copy", accel_group);
	gtk_widget_show (copy);
	gtk_container_add (GTK_CONTAINER (menuitem5_menu), copy);

	paste = gtk_image_menu_item_new_from_stock ("gtk-paste", accel_group);
	gtk_widget_show (paste);
	gtk_container_add (GTK_CONTAINER (menuitem5_menu), paste);

	help_menu = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_widget_show (help_menu);
	gtk_container_add (GTK_CONTAINER (main_menubar), help_menu);

	menuitem7_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (help_menu), menuitem7_menu);

	about1 = gtk_image_menu_item_new_from_stock ("gtk-about", accel_group);
	gtk_widget_show (about1);
	gtk_container_add (GTK_CONTAINER (menuitem7_menu), about1);

	main_notebook = gtk_notebook_new ();
	gtk_widget_show (main_notebook);
	gtk_box_pack_start (GTK_BOX (vbox7), main_notebook, TRUE, TRUE, 0);
	GTK_WIDGET_UNSET_FLAGS (main_notebook, GTK_CAN_FOCUS);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (main_notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (main_notebook), FALSE);

	main_hpane = gtk_hpaned_new ();
	gtk_widget_show (main_hpane);
	gtk_container_add (GTK_CONTAINER (main_notebook), main_hpane);
	gtk_paned_set_position (GTK_PANED (main_hpane), 1);

	contacts_vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (contacts_vbox);
	gtk_paned_pack1 (GTK_PANED (main_hpane), contacts_vbox, FALSE, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (contacts_vbox), 6);

	groups_combobox = gtk_combo_box_new_text ();
	gtk_widget_show (groups_combobox);
	gtk_box_pack_start (GTK_BOX (contacts_vbox), groups_combobox, FALSE, TRUE, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (groups_combobox), _("All"));
	gtk_combo_box_set_focus_on_click (GTK_COMBO_BOX (groups_combobox), FALSE);

	scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow2);
	gtk_box_pack_start (GTK_BOX (contacts_vbox), scrolledwindow2, TRUE, TRUE, 0);
	GTK_WIDGET_UNSET_FLAGS (scrolledwindow2, GTK_CAN_FOCUS);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_SHADOW_IN);

	contacts_treeview = gtk_tree_view_new ();
	gtk_widget_show (contacts_treeview);
	gtk_container_add (GTK_CONTAINER (scrolledwindow2), contacts_treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (contacts_treeview), FALSE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (contacts_treeview), FALSE);

	search_hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (search_hbox);
	gtk_box_pack_end (GTK_BOX (contacts_vbox), search_hbox, FALSE, TRUE, 0);

	search_entry_hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (search_entry_hbox);
	gtk_box_pack_start (GTK_BOX (search_hbox), search_entry_hbox, TRUE, TRUE, 0);

	search_label = gtk_label_new_with_mnemonic (_("_Search:"));
	gtk_widget_show (search_label);
	gtk_box_pack_start (GTK_BOX (search_entry_hbox), search_label, FALSE, FALSE, 0);
	gtk_misc_set_padding (GTK_MISC (search_label), 6, 0);

	search_entry = gtk_entry_new ();
	gtk_widget_show (search_entry);
	gtk_box_pack_start (GTK_BOX (search_entry_hbox), search_entry, TRUE, TRUE, 0);
	gtk_entry_set_activates_default (GTK_ENTRY (search_entry), TRUE);

	search_tab_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (search_hbox), search_tab_hbox, TRUE, TRUE, 0);

	symbols_radiobutton = gtk_radio_button_new_with_mnemonic (NULL, _("0-9#"));
	gtk_widget_show (symbols_radiobutton);
	gtk_box_pack_start (GTK_BOX (search_tab_hbox), symbols_radiobutton, TRUE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (symbols_radiobutton), symbols_radiobutton_group);
	symbols_radiobutton_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (symbols_radiobutton));

	atog_radiobutton = gtk_radio_button_new_with_mnemonic (NULL, _("A-G"));
	gtk_widget_show (atog_radiobutton);
	gtk_box_pack_start (GTK_BOX (search_tab_hbox), atog_radiobutton, TRUE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (atog_radiobutton), symbols_radiobutton_group);
	symbols_radiobutton_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (atog_radiobutton));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (atog_radiobutton), TRUE);

	hton_radiobutton = gtk_radio_button_new_with_mnemonic (NULL, _("H-N"));
	gtk_widget_show (hton_radiobutton);
	gtk_box_pack_start (GTK_BOX (search_tab_hbox), hton_radiobutton, TRUE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (hton_radiobutton), symbols_radiobutton_group);
	symbols_radiobutton_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (hton_radiobutton));

	otou_radiobutton = gtk_radio_button_new_with_mnemonic (NULL, _("O-U"));
	gtk_widget_show (otou_radiobutton);
	gtk_box_pack_start (GTK_BOX (search_tab_hbox), otou_radiobutton, TRUE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (otou_radiobutton), symbols_radiobutton_group);
	symbols_radiobutton_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (otou_radiobutton));

	vtoz_radiobutton = gtk_radio_button_new_with_mnemonic (NULL, _("V-Z"));
	gtk_widget_show (vtoz_radiobutton);
	gtk_box_pack_start (GTK_BOX (search_tab_hbox), vtoz_radiobutton, TRUE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (vtoz_radiobutton), symbols_radiobutton_group);
	symbols_radiobutton_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (vtoz_radiobutton));

	vbox3 = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox3);
	gtk_paned_pack2 (GTK_PANED (main_hpane), vbox3, TRUE, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (vbox3), 6);

	summary_vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (summary_vbox);
	gtk_box_pack_start (GTK_BOX (vbox3), summary_vbox, TRUE, TRUE, 0);

	preview_header_hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (preview_header_hbox);
	gtk_box_pack_start (GTK_BOX (summary_vbox), preview_header_hbox, FALSE, TRUE, 0);

	summary_name_label = gtk_label_new (_("<span><big><b>Welcome to Contacts</b></big></span>"));
	gtk_widget_show (summary_name_label);
	gtk_box_pack_start (GTK_BOX (preview_header_hbox), summary_name_label, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (summary_name_label, GTK_CAN_FOCUS);
	gtk_label_set_use_markup (GTK_LABEL (summary_name_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (summary_name_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (summary_name_label), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (summary_name_label), 6, 0);
	gtk_label_set_ellipsize (GTK_LABEL (summary_name_label), PANGO_ELLIPSIZE_END);

	photo_image = gtk_image_new_from_icon_name ("stock_contact", GTK_ICON_SIZE_DIALOG);
	gtk_widget_show (photo_image);
	gtk_box_pack_end (GTK_BOX (preview_header_hbox), photo_image, FALSE, TRUE, 6);
	gtk_misc_set_padding (GTK_MISC (photo_image), 1, 0);

	scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow3);
	gtk_box_pack_start (GTK_BOX (summary_vbox), scrolledwindow3, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow3), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	viewport1 = gtk_viewport_new (NULL, NULL);
	gtk_widget_show (viewport1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow3), viewport1);

	summary_table = gtk_table_new (1, 2, FALSE);
	gtk_widget_show (summary_table);
	gtk_container_add (GTK_CONTAINER (viewport1), summary_table);
	gtk_table_set_row_spacings (GTK_TABLE (summary_table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (summary_table), 6);

	summary_hbuttonbox = gtk_hbutton_box_new ();
	gtk_widget_show (summary_hbuttonbox);
	gtk_box_pack_end (GTK_BOX (vbox3), summary_hbuttonbox, FALSE, FALSE, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (summary_hbuttonbox), GTK_BUTTONBOX_SPREAD);

	new_button = gtk_button_new_from_stock ("gtk-new");
	gtk_widget_show (new_button);
	gtk_container_add (GTK_CONTAINER (summary_hbuttonbox), new_button);
	GTK_WIDGET_SET_FLAGS (new_button, GTK_CAN_DEFAULT);

	edit_button = gtk_button_new_from_stock ("gtk-edit");
	gtk_widget_show (edit_button);
	gtk_container_add (GTK_CONTAINER (summary_hbuttonbox), edit_button);
	gtk_widget_set_sensitive (edit_button, FALSE);
	GTK_WIDGET_SET_FLAGS (edit_button, GTK_CAN_DEFAULT);

	delete_button = gtk_button_new_from_stock ("gtk-delete");
	gtk_widget_show (delete_button);
	gtk_container_add (GTK_CONTAINER (summary_hbuttonbox), delete_button);
	gtk_widget_set_sensitive (delete_button, FALSE);
	GTK_WIDGET_SET_FLAGS (delete_button, GTK_CAN_DEFAULT);
	gtk_button_set_focus_on_click (GTK_BUTTON (delete_button), FALSE);

	label1 = gtk_label_new ("");
	gtk_widget_show (label1);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (main_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (main_notebook), 0), label1);

	vbox4 = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox4);
	gtk_container_add (GTK_CONTAINER (main_notebook), vbox4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox4), 6);

	scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow4);
	gtk_box_pack_start (GTK_BOX (vbox4), scrolledwindow4, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	viewport2 = gtk_viewport_new (NULL, NULL);
	gtk_widget_show (viewport2);
	gtk_container_add (GTK_CONTAINER (scrolledwindow4), viewport2);

	edit_table = gtk_table_new (1, 2, FALSE);
	gtk_widget_show (edit_table);
	gtk_container_add (GTK_CONTAINER (viewport2), edit_table);
	gtk_container_set_border_width (GTK_CONTAINER (edit_table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (edit_table), 6);

	hbuttonbox2 = gtk_hbutton_box_new ();
	gtk_widget_show (hbuttonbox2);
	gtk_box_pack_start (GTK_BOX (vbox4), hbuttonbox2, FALSE, TRUE, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox2), GTK_BUTTONBOX_END);

	add_field_button = gtk_button_new_with_mnemonic (_("_Add Field"));
	gtk_widget_show (add_field_button);
	gtk_container_add (GTK_CONTAINER (hbuttonbox2), add_field_button);
	GTK_WIDGET_SET_FLAGS (add_field_button, GTK_CAN_DEFAULT);

	remove_field_button = gtk_button_new_with_mnemonic (_("_Remove Field"));
	gtk_widget_show (remove_field_button);
	gtk_container_add (GTK_CONTAINER (hbuttonbox2), remove_field_button);
	GTK_WIDGET_SET_FLAGS (remove_field_button, GTK_CAN_DEFAULT);
	gtk_button_set_focus_on_click (GTK_BUTTON (remove_field_button), FALSE);

	edit_done_button = gtk_button_new_from_stock ("gtk-close");
	gtk_widget_show (edit_done_button);
	gtk_container_add (GTK_CONTAINER (hbuttonbox2), edit_done_button);
	GTK_WIDGET_SET_FLAGS (edit_done_button, GTK_CAN_DEFAULT);

	label2 = gtk_label_new ("");
	gtk_widget_show (label2);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (main_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (main_notebook), 1), label2);

	empty_notebook_page = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (empty_notebook_page);
	gtk_container_add (GTK_CONTAINER (main_notebook), empty_notebook_page);

	label3 = gtk_label_new ("");
	gtk_widget_show (label3);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (main_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (main_notebook), 2), label3);

	empty_notebook_page = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (empty_notebook_page);
	gtk_container_add (GTK_CONTAINER (main_notebook), empty_notebook_page);

	label4 = gtk_label_new ("");
	gtk_widget_show (label4);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (main_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (main_notebook), 3), label4);

	g_signal_connect ((gpointer) main_window, "destroy",
			G_CALLBACK (gtk_main_quit),
			NULL);
	g_signal_connect_swapped ((gpointer) main_window, "set_focus",
			G_CALLBACK (contacts_edit_set_focus_cb),
			GTK_OBJECT (remove_field_button));
	g_signal_connect ((gpointer) contacts_quit, "activate",
			G_CALLBACK (gtk_main_quit),
			NULL);
	g_signal_connect ((gpointer) contact_quit, "activate",
			G_CALLBACK (gtk_main_quit),
			NULL);
	g_signal_connect_swapped ((gpointer) cut, "activate",
			G_CALLBACK (contacts_cut_cb),
			GTK_OBJECT (main_window));
	g_signal_connect_swapped ((gpointer) copy, "activate",
			G_CALLBACK (contacts_copy_cb),
			GTK_OBJECT (main_window));
	g_signal_connect_swapped ((gpointer) paste, "activate",
			G_CALLBACK (contacts_paste_cb),
			GTK_OBJECT (main_window));
	g_signal_connect_swapped ((gpointer) about1, "activate",
			G_CALLBACK (contacts_about_cb),
			NULL);
	g_signal_connect_swapped ((gpointer) groups_combobox, "changed",
			G_CALLBACK (contacts_update_treeview),
			contacts_data);
	g_signal_connect_data ((gpointer) contacts_treeview, "key_press_event",
			G_CALLBACK (contacts_treeview_search_cb),
			GTK_OBJECT (search_entry),
			NULL, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
	g_signal_connect ((gpointer) search_entry, "changed",
			G_CALLBACK (contacts_search_changed_cb),
			contacts_data);
	g_signal_connect ((gpointer) symbols_radiobutton, "clicked",
			G_CALLBACK (contacts_search_changed_cb),
			contacts_data);
	g_signal_connect ((gpointer) atog_radiobutton, "clicked",
			G_CALLBACK (contacts_search_changed_cb),
			contacts_data);
	g_signal_connect ((gpointer) hton_radiobutton, "clicked",
			G_CALLBACK (contacts_search_changed_cb),
			contacts_data);
	g_signal_connect ((gpointer) otou_radiobutton, "clicked",
			G_CALLBACK (contacts_search_changed_cb),
			contacts_data);
	g_signal_connect ((gpointer) vtoz_radiobutton, "clicked",
			G_CALLBACK (contacts_search_changed_cb),
			contacts_data);
	g_signal_connect ((gpointer) remove_field_button, "clicked",
			G_CALLBACK (contacts_remove_field_cb),
			NULL);

	gtk_label_set_mnemonic_widget (GTK_LABEL (search_label), search_entry);

	gtk_widget_grab_focus (search_entry);
	gtk_widget_grab_default (edit_button);
	gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_group);

	ui->contact_delete = contact_delete;
	ui->contact_export = contact_export;
	ui->contact_menu = contact_menu;

	ui->contacts_import = contacts_import;
	ui->contacts_menu = contacts_menu;
	ui->contacts_treeview = contacts_treeview;

	ui->new_menuitem = new;
	ui->copy_menuitem = copy;
	ui->cut_menuitem = cut;
	ui->delete_menuitem = delete_menuitem;
	ui->delete_button = delete_button;
	ui->edit_menuitem = edit;
	ui->edit_button = edit_button;
	ui->edit_done_button = edit_done_button;
	ui->edit_groups = edit_groups;
	ui->edit_menu = edit_menu;
	ui->edit_table = edit_table;
	ui->main_menubar = main_menubar;
	ui->main_notebook = main_notebook;
	ui->main_window = main_window;
	ui->new_button = new_button;
	ui->paste_menuitem = paste;
	ui->photo_image = photo_image;
	ui->preview_header_hbox = preview_header_hbox;

	ui->add_field_button = add_field_button;
	ui->remove_field_button = remove_field_button;

	ui->search_entry = search_entry;
	ui->search_entry_hbox = search_entry_hbox;
	ui->search_hbox = search_hbox;
	ui->search_tab_hbox = search_tab_hbox;
	ui->groups_combobox = groups_combobox;

	ui->summary_hbuttonbox = summary_hbuttonbox;
	ui->summary_name_label = summary_name_label;
	ui->summary_table = summary_table;
	ui->summary_vbox = summary_vbox;

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
	gtk_widget_show (dialog_vbox1);

	vbox6 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox6);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox6, TRUE, TRUE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox6), 6);

	chooser_label = gtk_label_new (_("<span><b>Make a choice:</b></span>"));
	gtk_widget_show (chooser_label);
	gtk_box_pack_start (GTK_BOX (vbox6), chooser_label, FALSE, FALSE, 6);
	gtk_label_set_use_markup (GTK_LABEL (chooser_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (chooser_label), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (chooser_label), 6, 0);

	chooser_add_hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (chooser_add_hbox);
	gtk_box_pack_start (GTK_BOX (vbox6), chooser_add_hbox, FALSE, TRUE, 6);

	chooser_entry = gtk_entry_new ();
	gtk_widget_show (chooser_entry);
	gtk_box_pack_start (GTK_BOX (chooser_add_hbox), chooser_entry, TRUE, TRUE, 0);
	gtk_entry_set_activates_default (GTK_ENTRY (chooser_entry), TRUE);

	add_type_button = gtk_button_new_from_stock ("gtk-add");
	gtk_widget_show (add_type_button);
	gtk_box_pack_start (GTK_BOX (chooser_add_hbox), add_type_button, FALSE, FALSE, 0);
	GTK_WIDGET_SET_FLAGS (add_type_button, GTK_CAN_DEFAULT);
	gtk_button_set_focus_on_click (GTK_BUTTON (add_type_button), FALSE);

	scrolledwindow5 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow5);
	gtk_box_pack_start (GTK_BOX (vbox6), scrolledwindow5, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow5), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow5), GTK_SHADOW_IN);

	chooser_treeview = gtk_tree_view_new ();
	gtk_widget_show (chooser_treeview);
	gtk_container_add (GTK_CONTAINER (scrolledwindow5), chooser_treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (chooser_treeview), FALSE);

	dialog_action_area1 = GTK_DIALOG (chooser_dialog)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

	chooser_cancel_button = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (chooser_cancel_button);
	gtk_dialog_add_action_widget (GTK_DIALOG (chooser_dialog), chooser_cancel_button, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (chooser_cancel_button, GTK_CAN_DEFAULT);

	chooser_ok_button = gtk_button_new_from_stock ("gtk-ok");
	gtk_widget_show (chooser_ok_button);
	gtk_dialog_add_action_widget (GTK_DIALOG (chooser_dialog), chooser_ok_button, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (chooser_ok_button, GTK_CAN_DEFAULT);

	g_signal_connect ((gpointer) add_type_button, "clicked",
			G_CALLBACK (contacts_chooser_add_cb),
			data);

	gtk_widget_grab_focus (chooser_entry);
	gtk_widget_grab_default (add_type_button);

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
