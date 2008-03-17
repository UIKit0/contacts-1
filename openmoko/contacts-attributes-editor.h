/*
 * OpenMoko Contacts
 *
 * Copyright (C) 2007 OpenMoko, Inc.
 * 
 * OpenMoko Contacts is free software: you can redistribute it and/or modify it
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

#ifndef _CONTACTS_ATTRIBUTES_EDITOR_H_
#define _CONTACTS_ATTRIBUTES_EDITOR_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "contacts-attribute-store.h"

G_BEGIN_DECLS

#define CONTACTS_TYPE_ATTRIBUTES_EDITOR             (contacts_attributes_editor_get_type ())
#define CONTACTS_ATTRIBUTES_EDITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONTACTS_TYPE_ATTRIBUTES_EDITOR, ContactsAttributesEditor))
#define CONTACTS_ATTRIBUTES_EDITOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CONTACTS_TYPE_ATTRIBUTES_EDITOR, ContactsAttributesEditorClass))
#define CONTACTS_IS_ATTRIBUTES_EDITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CONTACTS_TYPE_ATTRIBUTES_EDITOR))
#define CONTACTS_IS_ATTRIBUTES_EDITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CONTACTS_TYPE_ATTRIBUTES_EDITOR))
#define CONTACTS_ATTRIBUTES_EDITOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CONTACTS_TYPE_ATTRIBUTES_EDITOR, ContactsAttributesEditorClass))

typedef struct _ContactsAttributesEditorClass ContactsAttributesEditorClass;
typedef struct _ContactsAttributesEditor ContactsAttributesEditor;

struct _ContactsAttributesEditorClass
{
  GtkVBoxClass parent_class;
};

struct _ContactsAttributesEditor
{
  GtkVBox parent_instance;
};

GType contacts_attributes_editor_get_type (void) G_GNUC_CONST;

GtkWidget * contacts_attributes_editor_new (ContactsAttributeStore *store, const gchar *type);
void contacts_attributes_editor_set_editable (ContactsAttributesEditor *editor,
                                             gboolean editable);
void contacts_attributes_editor_set_attribute_store (ContactsAttributesEditor *editor,
                                     ContactsAttributeStore *store);
void contacts_attributes_editor_set_type (ContactsAttributesEditor *editor, const gchar *type);


G_END_DECLS

#endif /* _CONTACTS_ATTRIBUTES_EDITOR_H_ */
