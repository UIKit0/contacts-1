
#include <glib.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>
#include "contacts-defs.h"

void contacts_update_treeview ();

void contacts_selection_cb (GtkTreeSelection * selection, ContactsData *data);

void contacts_new_cb (GtkWidget *source, ContactsData *data);

void contacts_edit_cb (GtkWidget *source, ContactsData *data);

void contacts_delete_cb (GtkWidget *source, ContactsData *data);

void contacts_import_cb (GtkWidget *source, ContactsData *data);

void contacts_copy_cb (GtkWindow *main_window);

void contacts_cut_cb (GtkWindow *main_window);

void contacts_paste_cb (GtkWindow *main_window);

void contacts_about_cb (GtkWidget *dialog);

gboolean contacts_is_row_visible_cb (GtkTreeModel * model, GtkTreeIter * iter,
				     GHashTable *contacts_table);

gint contacts_sort_treeview_cb (GtkTreeModel * model, GtkTreeIter * a,
				GtkTreeIter * b, gpointer user_data);
