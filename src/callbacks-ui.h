
#include <glib.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>

void quit ();

gboolean is_row_visible (GtkTreeModel * model, GtkTreeIter * iter,
			 gpointer data);

gint sort_treeview_func (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b,
			 gpointer user_data);

void update_treeview ();

void display_contact_summary (EContact *contact);

void contact_selected (GtkTreeSelection * selection);

void new_contact ();

void edit_contact ();

void delete_contact ();

void copy (GtkWindow *main_window);

void cut (GtkWindow *main_window);

void paste (GtkWindow *main_window);

void about ();

void edit_done (GtkWidget *button, EContact *contact);
