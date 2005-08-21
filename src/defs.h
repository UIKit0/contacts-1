
#include <libebook/e-book.h>
#include <gtk/gtk.h>

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
	EContact *contact;
	GtkTreeIter iter;
} EContactListHash;

typedef struct {
	EContact *contact;
	EVCardAttribute *attr;
} EContactChangeData;

#define NO_IMAGE 1
