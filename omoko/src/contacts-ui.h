#include "contacts-defs.h"
#include "config.h"

#ifndef FRONTEND_HEADER
#error No front-end header defined
#else
#include FRONTEND_HEADER
#endif


void contacts_setup_ui (ContactsData *data);
void contacts_remove_labels_from_focus_chain (GtkContainer *container);

void contacts_display_summary (EContact *contact, ContactsData *data);

void contacts_set_available_options (ContactsData *data, gboolean new, gboolean open,
				     gboolean delete);
