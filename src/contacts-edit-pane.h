
#include <glib.h>
#include <gtk/gtk.h>
#include "contacts-defs.h"

void contacts_edit_pane_set_focus_cb (GtkWindow *window, GtkWidget *widget,
				      gpointer data);

void contacts_edit_pane_show (ContactsData *data);

void contacts_remove_field_cb (GtkWidget *button, gpointer data);

GtkWidget *contacts_label_widget_new (EContact *contact, EVCardAttribute *attr,
				      const gchar *name, gboolean multi_line);

void contacts_remove_edit_widgets_cb (GtkWidget *widget, gpointer data);

const gchar *contacts_field_pretty_name (const ContactsField *field);

void contacts_append_to_edit_table (GtkTable *table,
				    GtkWidget *label, GtkWidget *edit);

const ContactsField *contacts_get_contacts_field (const gchar *vcard_field);

const gchar **contacts_get_field_types (const gchar *attr_name);
