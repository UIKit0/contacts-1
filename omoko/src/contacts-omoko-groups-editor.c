/*
 * contacts-omoko-groups-editor.c
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

#include "contacts-defs.h"
#include "contacts-omoko.h"
#include "contacts-main.h"

static void create_chooser_dialog (ContactsData *data);

void
moko_open_groups_editor (GtkWidget *widget, ContactsData *data)
{
	if (!MOKO_IS_DIALOG_WINDOW (data->ui->chooser_dialog))
		create_chooser_dialog (data);
	moko_dialog_window_run (MOKO_DIALOG_WINDOW (data->ui->chooser_dialog));
}

static void
create_chooser_dialog (ContactsData *data)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *widget;
	GtkWidget *groups_vbox;

	dialog = GTK_WIDGET (moko_dialog_window_new ());
	moko_dialog_window_set_title (MOKO_DIALOG_WINDOW (dialog), _("Group Membership") );

	vbox = gtk_vbox_new (FALSE, 0);
	moko_dialog_window_set_contents (MOKO_DIALOG_WINDOW (dialog), vbox);


	widget = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (widget), 24, 24, 24, 24);
	gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);

	groups_vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (widget), groups_vbox, TRUE, TRUE, 0);

	GtkWidget *contact_label = gtk_label_new ("<span size=\"large\"><b>Contact Name</b></span>");
	gtk_box_pack_start (GTK_BOX (groups_vbox), contact_label, FALSE, FALSE, 12);
	gtk_label_set_use_markup (GTK_LABEL (contact_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (contact_label), 0, 0);


	widget = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (groups_vbox), widget, TRUE, TRUE, 0);

	GList *cur;
	for (cur = data->contacts_groups; cur; cur = g_list_next (cur))
	{
		widget = gtk_check_button_new_with_label (cur->data);
		gtk_box_pack_start (GTK_BOX (groups_vbox), widget, TRUE, TRUE, 6);
	}

	gtk_widget_show_all (vbox);

	data->ui->chooser_dialog = dialog;

}


