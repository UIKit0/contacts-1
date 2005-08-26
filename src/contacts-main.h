
#include <libebook/e-book.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

void contacts_update_treeview (GtkWidget *source);

void contacts_display_summary (EContact *contact, GladeXML *xml);
