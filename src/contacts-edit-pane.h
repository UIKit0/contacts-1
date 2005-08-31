
#include <glib.h>
#include <gtk/gtk.h>
#include "contacts-defs.h"

void contacts_edit_pane_show (ContactsData *data, gboolean new);

void contacts_remove_field_cb (GtkWidget *button, gpointer data);

GtkWidget *contacts_label_widget_new (EContact *contact, EVCardAttribute *attr,
				      const gchar *name, gboolean multi_line);

void contacts_remove_edit_widgets_cb (GtkWidget *widget, gpointer data);

void contacts_append_to_edit_table (GtkTable *table,
				    GtkWidget *label, GtkWidget *edit);
