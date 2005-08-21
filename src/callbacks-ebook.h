
#include <glib.h>
#include <libebook/e-book.h>

void contacts_added_cb (EBookView *book_view, const GList *contacts);

void contacts_changed_cb (EBookView *book_view, const GList *contacts);

void contacts_removed_cb (EBookView *book_view, const GList *ids);
