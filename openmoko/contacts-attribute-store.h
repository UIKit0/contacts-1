/*
 * hito
 * Copyright (C) OpenedHand Ltd 2007 <info@openedhand.com>
 * 
 * hito is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * hito is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CONTACTS_ATTRIBUTE_STORE_H_
#define _CONTACTS_ATTRIBUTE_STORE_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>

G_BEGIN_DECLS

#define CONTACTS_TYPE_ATTRIBUTE_STORE             (contacts_attribute_store_get_type ())
#define CONTACTS_ATTRIBUTE_STORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONTACTS_TYPE_ATTRIBUTE_STORE, ContactsAttributeStore))
#define CONTACTS_ATTRIBUTE_STORE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CONTACTS_TYPE_ATTRIBUTE_STORE, ContactsAttributeStoreClass))
#define CONTACTS_IS_ATTRIBUTE_STORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CONTACTS_TYPE_ATTRIBUTE_STORE))
#define CONTACTS_IS_ATTRIBUTE_STORE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CONTACTS_TYPE_ATTRIBUTE_STORE))
#define CONTACTS_ATTRIBUTE_STORE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CONTACTS_TYPE_ATTRIBUTE_STORE, ContactsAttributeStoreClass))

typedef struct _ContactsAttributeStoreClass ContactsAttributeStoreClass;
typedef struct _ContactsAttributeStore ContactsAttributeStore;

struct _ContactsAttributeStoreClass
{
  GtkListStoreClass parent_class;
};

struct _ContactsAttributeStore
{
  GtkListStore parent_instance;
};

enum {
  ATTR_POINTER_COLUMN,
  ATTR_NAME_COLUMN,
  ATTR_TYPE_COLUMN,
  ATTR_VALUE_COLUMN
};

GType contacts_attribute_store_get_type (void) G_GNUC_CONST;


GtkTreeModel * contacts_attribute_store_new ();
void contacts_attribute_store_set_vcard (ContactsAttributeStore *store, EVCard *card);

G_END_DECLS

#endif /* _CONTACTS_ATTRIBUTE_STORE_H_ */
