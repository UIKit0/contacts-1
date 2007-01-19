#include <string.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>
#include "contacts-contact-pane.h"

G_DEFINE_TYPE (ContactsContactPane, contacts_contact_pane, GTK_TYPE_VBOX);

#define CONTACT_PANE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CONTACTS_TYPE_CONTACT_PANE, ContactsContactPanePrivate))


struct _ContactsContactPanePrivate
{
  EBookView *bookview;
  EContact *contact;
  gboolean dirty;
  gboolean editable;
};

typedef struct {
  char *vcard_field; /* vCard field name */
  char *display_name; /* Human-readable name for display */
  char *icon; /* Icon name for the menu */
  gboolean unique; /* If there can be only one of this field */
  /* TODO: add an extra changed callback so that N handler can update FN, etc */
} FieldInfo;

static GQuark attr_quark = 0;
static GQuark field_quark = 0;

static const FieldInfo fields[] = {
  { EVC_FN, "Name", NULL, TRUE },
  { EVC_EMAIL, "Email", "stock_mail", FALSE },
  { EVC_X_JABBER, "Jabber", GTK_STOCK_MISSING_IMAGE, FALSE },
};

/*
 * TODO: 
 * - add EBookView and listen for changes
 */

/* Implementation */

/*
 * From a contact return a list of all EVCardAttributes that are the specified
 * name.
 */
static GList *
contact_get_attributes (EContact *contact, const char *name)
{
  GList *attrs = NULL, *l;
  
  g_return_val_if_fail (E_IS_CONTACT (contact), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  
  for (l = e_vcard_get_attributes (E_VCARD (contact)); l; l = l->next) {
    EVCardAttribute *attr;
    const char *n;
    
    attr = (EVCardAttribute *) l->data;
    n = e_vcard_attribute_get_name (attr);
    
    if (strcmp (n, name) == 0)
      attrs = g_list_prepend (attrs, attr);
  }
  
  return g_list_reverse (attrs);
}

static void
field_changed (GtkWidget *entry, ContactsContactPane *pane)
{
  EVCardAttribute *attr;
  FieldInfo *info;
  const char *value;

  attr = g_object_get_qdata (G_OBJECT (entry), attr_quark);
  g_assert (attr);

  info = g_object_get_qdata (G_OBJECT (entry), field_quark);
  g_assert (info);

  value = gtk_entry_get_text (GTK_ENTRY (entry));
  
  /* TODO: this only handles single-valued attributes at the moment */
  e_vcard_attribute_remove_values (attr);
  e_vcard_attribute_add_value (attr, value);

  pane->priv->dirty = TRUE;
}

static GtkWidget *
make_widget (ContactsContactPane *pane, EVCardAttribute *attr, const FieldInfo *info, GtkSizeGroup *size)
{
  GtkWidget *box, *type_label, *image, *value;

  box = gtk_hbox_new (FALSE, 4);

  /* The label */
  type_label = gtk_label_new (g_strconcat (info->display_name, ":", NULL));
  if (size)
    gtk_size_group_add_widget (size, type_label);
  gtk_box_pack_start (GTK_BOX (box), type_label, FALSE, FALSE, 4);

  if (pane->priv->editable || info->icon) {
    /* TODO: hook up an event box for clicks */
    if (pane->priv->editable) {
      /* TODO: use the correct image */
      image = gtk_image_new_from_icon_name (GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_MENU);
    } else {
      image = gtk_image_new_from_icon_name (info->icon, GTK_ICON_SIZE_MENU);
    }
    gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 4);
  }
  
  /* The value field itself */
  if (pane->priv->editable) {
    value = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (value), e_vcard_attribute_get_value (attr));
    g_object_set_qdata (G_OBJECT (value), attr_quark, attr);
    g_object_set_qdata (G_OBJECT (value), field_quark, (gpointer)info);
    g_signal_connect (value, "changed", G_CALLBACK (field_changed), pane);
  } else {
    value = gtk_label_new (e_vcard_attribute_get_value (attr));
    gtk_misc_set_alignment (GTK_MISC (value), 0.0, 0.5);
  }
  gtk_box_pack_start (GTK_BOX (box), value, TRUE, TRUE, 4);

  gtk_widget_show_all (box);
  return box;
}

/*
 * Update the widgets, called when the contact or editable mode has changed.
 */
static void
update_ui (ContactsContactPane *pane)
{
  GtkSizeGroup *size;
  int i;

  g_assert (CONTACTS_IS_CONTACT_PANE (pane));

  /* First, clear the pane */
  gtk_container_foreach (GTK_CONTAINER (pane),
                         (GtkCallback)gtk_widget_destroy, NULL);
  

  if (pane->priv->contact == NULL) {
    if (pane->priv->editable) {
      g_warning (G_STRLOC ": TODO: create blank contact if new=true");
      return;
    } else {
      return;
    }
  }

  size = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  for (i = 0; i < G_N_ELEMENTS (fields); i++) {
    const FieldInfo *info;
    EVCardAttribute *attr;
    GtkWidget *w;

    info = &fields[i];
    if (info->unique) {
      /* Fast path unique fields, no need to search the entire contact */
      attr = e_vcard_get_attribute (E_VCARD (pane->priv->contact), info->vcard_field);
      if (attr) {
        w = make_widget (pane, attr, info, size);
        gtk_box_pack_start (GTK_BOX (pane), w, FALSE, FALSE, 4);
      }
    } else {
      GList *attrs, *l;
      attrs = contact_get_attributes (pane->priv->contact, info->vcard_field);
      for (l = attrs; l ; l = g_list_next (l)) {
        EVCardAttribute *attr = l->data;
        w = make_widget (pane, attr, info, size);
        gtk_box_pack_start (GTK_BOX (pane), w, FALSE, FALSE, 4);        
      }
    }
  }
  
  g_object_unref (size);
}

/* GObject methods */

static void
contacts_contact_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
contacts_contact_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
contacts_contact_pane_dispose (GObject *object)
{
  if (G_OBJECT_CLASS (contacts_contact_pane_parent_class)->dispose)
    G_OBJECT_CLASS (contacts_contact_pane_parent_class)->dispose (object);
}

static void
contacts_contact_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (contacts_contact_pane_parent_class)->finalize (object);
}

static void
contacts_contact_pane_class_init (ContactsContactPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ContactsContactPanePrivate));

  object_class->get_property = contacts_contact_pane_get_property;
  object_class->set_property = contacts_contact_pane_set_property;
  object_class->dispose = contacts_contact_pane_dispose;
  object_class->finalize = contacts_contact_pane_finalize;

  /* TODO: properties for editable and contact */
  
  /* Initialise the quarks */
  attr_quark = g_quark_from_static_string("contact-pane-attribute");
  field_quark = g_quark_from_static_string("contact-pane-fieldinfo");
}

static void
contacts_contact_pane_init (ContactsContactPane *self)
{
  self->priv = CONTACT_PANE_PRIVATE (self);

}


/* Public API */

GtkWidget *
contacts_contact_pane_new (void)
{
  return g_object_new (CONTACTS_TYPE_CONTACT_PANE, NULL);
}

void
contacts_contact_pane_set_editable (ContactsContactPane *pane, gboolean editable)
{
  g_return_if_fail (CONTACTS_IS_CONTACT_PANE (pane));

  if (pane->priv->editable != editable) {
    pane->priv->editable = editable;
    update_ui (pane);
  }
}

void
contacts_contact_pane_set_book_view (ContactsContactPane *pane, EBookView *view)
{
  g_return_if_fail (CONTACTS_IS_CONTACT_PANE (pane));
  g_return_if_fail (E_IS_BOOK_VIEW (view));
  
  if (pane->priv->bookview) {
    g_object_unref (pane->priv->bookview);
  }
  
  pane->priv->bookview = g_object_ref (view);
}

static void
on_commit_cb (EBook *book, EBookStatus status, gpointer closure)
{
  if (status != E_BOOK_ERROR_OK) {
    /* TODO: show error dialog */
    g_warning ("Cannot commit contact: %d", status);
  }
}

void
contacts_contact_pane_set_contact (ContactsContactPane *pane, EContact *contact)
{
  ContactsContactPanePrivate *priv;

  g_return_if_fail (CONTACTS_IS_CONTACT_PANE (pane));

  priv = pane->priv;

  /* check to see if the contact is the same as the current one */
  if (priv->contact) {
    if (strcmp (e_contact_get_const (contact, E_CONTACT_UID),
                e_contact_get_const (priv->contact, E_CONTACT_UID)) == 0) {
      return;
    }
  }

  if (priv->contact) {
    if (priv->dirty && priv->bookview) {
      e_book_async_commit_contact (e_book_view_get_book (priv->bookview),
                                   priv->contact,
                                   on_commit_cb, pane);
    }
    g_object_unref (priv->contact);
  }

  if (contact) {
    priv->contact = g_object_ref (contact);
  } else {
    priv->contact = NULL;
  }
  priv->dirty = FALSE;

  update_ui (pane);
}
