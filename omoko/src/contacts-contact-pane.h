#ifndef _CONTACTS_CONTACT_PANE
#define _CONTACTS_CONTACT_PANE

#include <gtk/gtkvbox.h>
#include <libebook/e-book.h>

G_BEGIN_DECLS

#define CONTACTS_TYPE_CONTACT_PANE contacts_contact_pane_get_type()

#define CONTACTS_CONTACT_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CONTACTS_TYPE_CONTACT_PANE, ContactsContactPane))

#define CONTACTS_CONTACT_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CONTACTS_TYPE_CONTACT_PANE, ContactsContactPaneClass))

#define CONTACTS_IS_CONTACT_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CONTACTS_TYPE_CONTACT_PANE))

#define CONTACTS_IS_CONTACT_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CONTACTS_TYPE_CONTACT_PANE))

#define CONTACTS_CONTACT_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CONTACTS_TYPE_CONTACT_PANE, ContactsContactPaneClass))

typedef struct _ContactsContactPanePrivate ContactsContactPanePrivate;

typedef struct {
  GtkVBox parent;
  ContactsContactPanePrivate *priv;
} ContactsContactPane;

typedef struct {
  GtkVBoxClass parent_class;
} ContactsContactPaneClass;

GType contacts_contact_pane_get_type (void);

GtkWidget* contacts_contact_pane_new (void);

void contacts_contact_pane_set_book_view (ContactsContactPane *pane, EBookView *view);
void contacts_contact_pane_set_contact (ContactsContactPane *pane, EContact *contact);
void contacts_contact_pane_set_editable (ContactsContactPane *pane, gboolean editable);

G_END_DECLS

#endif /* _CONTACTS_CONTACT_PANE */
