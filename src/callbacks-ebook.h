
#include <glib.h>
#include <libebook/e-book.h>
#include "defs.h"

void contacts_added_cb (EBookView *book_view, const GList *contacts,
			ContactsData *data);

void contacts_changed_cb (EBookView *book_view, const GList *contacts,
			  ContactsData *data);

void contacts_removed_cb (EBookView *book_view, const GList *ids,
			  ContactsData *data);
