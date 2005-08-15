
#include <libebook/e-book.h>
#include <gtk/gtk.h>

typedef enum {
	NAME,
	UID,
	LAST
} COLUMNS;

typedef struct {
	EContact *contact;
	GtkTreeIter iter;
} EContactListHash;

typedef struct {
	EContact *contact;
	EContactField field_id;
} EContactChangeData;

#define NO_IMAGE 1
