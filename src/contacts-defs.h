
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
	CHOOSER_TICK_COL = 0,
	CHOOSER_NAME_COL
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
	GtkWidget *widget; /* The widget containing the data */
} EContactChangeData;

typedef struct {
	const gchar *attr_name;
	EVCardAttributeParam *param;
} EContactTypeChangeData;

typedef struct {
	const gchar *vcard_field;
	EContactField econtact_field; /* >0, gets used for pretty name */
	const gchar *pretty_name; /* Always takes priority over above */
	gboolean multi_line;
	guint priority;
	gboolean unique;
} ContactsField;

/* Contains structured field data
 * TODO: Replace this with a less ugly construct?
 */

typedef struct {
	const gchar *attr_name;
	guint field;
	const gchar *subfield_name;
	gboolean multiline;
} ContactsStructuredField;

#define REQUIRED 100	/* Contacts with priority <= REQUIRED have to be shown
			 * when creating a new contact.
			 */

#define NO_IMAGE 1	/* For the image-chooser dialog */

#endif
