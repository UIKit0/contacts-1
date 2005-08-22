
#include <libebook/e-book.h>
#include <gtk/gtk.h>

#ifndef DEFS_H
#define DEFS_H

enum {
	ADR_POBOX = 0,
	ADR_EXT,	/* Multiple line */
	ADR_STREET,	/* Multiple line */
	ADR_LOCAL,
	ADR_REGION,
	ADR_CODE,
	ADR_COUNTRY
};

enum {
	CONTACT_NAME_COL,
	CONTACT_UID_COL,
	CONTACT_LAST_COL
};

enum {
	TYPE_CHOSEN_COL = 0,
	TYPE_NAME_COL
};

typedef struct {
	EBook *book;
	EContact *contact;
	GHashTable *contacts_table;
	GList *contacts_groups;
	GladeXML *xml;
} ContactsData;

typedef struct {
	EContact *contact;
	GtkTreeIter iter;
	GladeXML *xml;
} EContactListHash;

typedef struct {
	EContact *contact;
	EVCardAttribute *attr;
} EContactChangeData;

#define NO_IMAGE 1

#endif
