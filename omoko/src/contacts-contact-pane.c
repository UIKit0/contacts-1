/*
 * Copyright (C) 2006-2007 by OpenMoko, Inc.
 * Written by OpenedHand Ltd <info@openedhand.com>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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

static gchar *email_types[] = {
  "Work",
  "Home",
  "Other",
  NULL
};

static gchar *phone_types[] = {
  "Work",
  "Home",
  "Mobile",
  "Fax",
  "Pager",
  "Other",
  NULL
};

typedef struct {
  char *vcard_field; /* vCard field name */
  char *display_name; /* Human-readable name for display */
  char *icon; /* Icon name for the menu */
  gboolean unique; /* If there can be only one of this field */
  char *format; /* format string */
  gchar **types;
  /* TODO: add an extra changed callback so that N handler can update FN, etc */
} FieldInfo;

static GQuark attr_quark = 0;
static GQuark field_quark = 0;

static FieldInfo fields[] = {
  { EVC_FN, "Name", NULL, TRUE, "<big><b>%s</b></big>", NULL },
  { EVC_ORG, "Organization", NULL, TRUE, "<span size=\"x-small\">%s</span>", NULL },
  { EVC_EMAIL, "E-Mail", "stock_mail", FALSE, NULL, email_types },
  { EVC_TEL, "Telephone", NULL, FALSE, NULL, phone_types },
  { EVC_X_JABBER, "Jabber", GTK_STOCK_MISSING_IMAGE, FALSE, NULL, email_types },
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

/*
 * Strip empty attributes from a vcard
 */
static void
strip_empty_attributes (EVCard *card)
{
  GList *attrs, *values;
  gboolean remove;
  EVCardAttribute *attribute;

  attrs = e_vcard_get_attributes (card);
  while (attrs) {
    attribute = attrs->data;
    remove = TRUE;
    values = e_vcard_attribute_get_values (attrs->data);

    while (values) {
      if (g_utf8_strlen (values->data, -1) > 0) {
        remove = FALSE;
        break;
      }
      values = g_list_next (values);
    }

    attrs = g_list_next (attrs);
    if (remove)
      e_vcard_remove_attribute (card, attribute);
  }
}

/*
 * Set the entry to display as a blank field (i.e. with the display name in the
 * "background" of the widget)
 */
static void
field_set_blank (GtkEntry *entry, FieldInfo *info)
{
  /* TODO: use some colour from the theme */
  GdkColor gray;
  gdk_color_parse ("LightGray", &gray);
  gtk_entry_set_text (GTK_ENTRY (entry), info->display_name);
  gtk_widget_modify_text (GTK_WIDGET (entry), GTK_STATE_NORMAL, &gray);
}

/*
 * Callback for when a field entry is modified
 */
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

  /* don't save the value if we're just displaying the field name */
  if (value && !strcmp (info->display_name, value))
    return;
  
  /* TODO: this only handles single-valued attributes at the moment */
  e_vcard_attribute_remove_values (attr);
  e_vcard_attribute_add_value (attr, value);

  pane->priv->dirty = TRUE;
}

/*
 * Callback for when a field entry recieves focus
 */
static gboolean
field_focus_in (GtkWidget *entry, GdkEventFocus *event, FieldInfo *info)
{
  if (!strcmp (gtk_entry_get_text (GTK_ENTRY (entry)), info->display_name)) {
    /* TODO: use some colour from the theme */
    GdkColor gray;
    gdk_color_parse ("Black", &gray);
    gtk_entry_set_text (GTK_ENTRY (entry), info->display_name);
    gtk_widget_modify_text (GTK_WIDGET (entry), GTK_STATE_NORMAL, &gray);

    gtk_entry_set_text (GTK_ENTRY (entry), "");
  }
  return FALSE;
}

/*
 * Callback for when a field entry looses focus
 */
static gboolean
field_focus_out (GtkWidget *entry, GdkEventFocus *event, FieldInfo *info)
{
  if (!strcmp (gtk_entry_get_text (GTK_ENTRY (entry)), "")) {
    field_set_blank (GTK_ENTRY (entry), info);
  }
  return FALSE;
}

/*
 * Callback for when the type menu needs to be shown
 */
static void
show_menu (GtkWidget *widget, GdkEventButton *event, GtkMenu *menu)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, gtk_get_current_event_time());
}

/*
 * Convenience function to set the type property of a vcard attribute
 */
static void
set_type (EVCardAttribute *attr, gchar *type)
{
  GList *params;
  EVCardAttributeParam *p = NULL;

  
  for (params = e_vcard_attribute_get_params (attr); params;
      params = g_list_next (params))
    if (!strcmp (e_vcard_attribute_param_get_name (params->data), "TYPE"))
    {
      p = params->data;
      break;
    }

  /* if we didn't find the type param */
  if (p == NULL)
  {
    p = e_vcard_attribute_param_new ("TYPE");
    e_vcard_attribute_add_param (attr, p);
  }

  /* FIXME: we can only deal with one attribute type value at the moment */
  e_vcard_attribute_param_remove_values (p);
  e_vcard_attribute_param_add_value (p, type);
}

/*
 * Convenience function to get the type property of a vcard attribute
 */
static gchar*
get_type (EVCardAttribute *attr)
{
  GList *list;
  list = e_vcard_attribute_get_param (attr, "TYPE");
  /* FIXME: we can only deal with one attribute type value at the moment */
  return (list) ? list->data : NULL;
}

/*
 * Callback for when a menuitem in the attribute type menu is activated
 */
static void
set_type_cb (GtkWidget *widget, EVCardAttribute *attr)
{
  /* TODO: use quarks here */
  gchar *new_type = gtk_combo_box_get_active_text (GTK_COMBO_BOX (widget));
  ContactsContactPane *pane = g_object_get_data (G_OBJECT (widget), "contact-pane");

  set_type (attr, new_type);
  pane->priv->dirty = TRUE;
}

static GtkWidget *
make_widget (ContactsContactPane *pane, EVCardAttribute *attr, FieldInfo *info, GtkSizeGroup *size)
{
  GtkWidget *box, *type_label = NULL, *image, *event_box, *value, *menu;
  gchar *attr_value = NULL, *escaped_str, *type, *s;
  gint i = 0;

  box = gtk_hbox_new (FALSE, 0);

  type = get_type (attr);
  if (type == NULL && info->types != NULL)
    type = info->types[0];

  /* insert add/remove buttons */
  if (pane->priv->editable && !info->unique)
  {
    /* need to use an alignment here to stop the button expanding vertically */
    GtkWidget *btn, *alignment;
    btn = gtk_button_new ();
    gtk_widget_set_name (btn, "addbutton");
    alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_widget_set_size_request (btn, 24, 21);
    gtk_container_add (GTK_CONTAINER (alignment), btn);
    gtk_box_pack_start (GTK_BOX (box), alignment, FALSE, FALSE, 0);

    btn = gtk_button_new ();
    gtk_widget_set_name (btn, "removebutton");
    gtk_widget_set_size_request (btn, 24, 21);
    alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (alignment), btn);
    gtk_box_pack_start (GTK_BOX (box), alignment, FALSE, FALSE, 0);
  }


  /* The label (if required) */
  if (!info->unique && !pane->priv->editable) {
    type_label = gtk_label_new (type);
    if (size)
      gtk_size_group_add_widget (size, type_label);
    gtk_box_pack_start (GTK_BOX (box), type_label, FALSE, FALSE, 4);
  }
  if (info->types && pane->priv->editable)
  {
    GtkWidget *combo;
    combo = gtk_combo_box_new_text ();
    gtk_widget_set_size_request (combo, -1, 46);
    i = 0;
    for (s = info->types[i]; (s = info->types[i]); i++) {
      gtk_combo_box_append_text (combo, s);
      if (!strcmp (s, type))
        gtk_combo_box_set_active (combo, i);
    }
    g_object_set_data (G_OBJECT (combo), "contact-pane", pane);
    g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (set_type_cb), attr);
    if (size)
      gtk_size_group_add_widget (size, combo);
    gtk_box_pack_start (GTK_BOX (box), combo, FALSE, FALSE, 4);
  }


#if 0
  /* Field type selector */
  if (pane->priv->editable || info->icon) {
    if (pane->priv->editable && !info->unique) {
      /* TODO: use the correct image */
      image = gtk_image_new_from_icon_name (GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_MENU);
    } else {
      image = gtk_image_new_from_icon_name (info->icon, GTK_ICON_SIZE_MENU);
    }
    event_box = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (event_box), GTK_WIDGET (image));
    gtk_box_pack_start (GTK_BOX (box), event_box, FALSE, FALSE, 4);

    /* build selecter menu */
    if (info->types && pane->priv->editable)
    {
      menu = gtk_menu_new ();
      i = 0;
      for (s = info->types[i]; (s = info->types[i]); i++)
      {
        GtkWidget *menu_item = gtk_menu_item_new_with_label (s);
        /* TODO: use quarks here */
        g_object_set_data (G_OBJECT (menu_item), "contact-attribute-type-value", s);
        if (type_label)
          g_object_set_data (G_OBJECT (menu_item), "contact-attribute-type-label", type_label);
        g_object_set_data (G_OBJECT (menu_item), "contact-pane", pane);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
        g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (set_type_menu_cb), attr);
      }
      g_signal_connect (event_box, "button-release-event", G_CALLBACK (show_menu), menu);
      gtk_widget_show_all (menu);

    }
  }
#endif
  
  /* The value field itself */

  /* FIXME: fix this for multivalued attributes */
  attr_value = e_vcard_attribute_get_value (attr);

  if (pane->priv->editable) {
    value = gtk_entry_new ();
    if (attr_value)
      gtk_entry_set_text (GTK_ENTRY (value), attr_value);
    else
    {
      /* this is a field that doesn't have a value yet */
      field_set_blank (GTK_ENTRY (value), info);
    }

    g_object_set_qdata (G_OBJECT (value), attr_quark, attr);
    g_object_set_qdata (G_OBJECT (value), field_quark, (gpointer)info);
    g_signal_connect (value, "changed", G_CALLBACK (field_changed), pane);
    g_signal_connect (value, "focus-in-event", G_CALLBACK (field_focus_in), info);
    g_signal_connect (value, "focus-out-event", G_CALLBACK (field_focus_out), info);
  } else {
    if (info->format)
    {
      escaped_str = g_markup_printf_escaped (info->format, attr_value);
      value = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (value), escaped_str);
      g_free (escaped_str);
    }
    else
      value = gtk_label_new (attr_value);
    gtk_misc_set_alignment (GTK_MISC (value), 0.0, 0.5);
  }
  gtk_box_pack_start (GTK_BOX (box), value, TRUE, TRUE, 4);

  gtk_widget_show_all (box);
  g_free (attr_value);
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
      pane->priv->contact = e_contact_new ();
      return;
    } else {
      GtkWidget *w;
      w = gtk_label_new ("No contact to display");
      gtk_widget_show (w);
      gtk_box_pack_start (GTK_BOX (pane), w, TRUE, TRUE, 0);
      return;
    }
  }

  size = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  for (i = 0; i < G_N_ELEMENTS (fields); i++) {
    FieldInfo *info;
    EVCardAttribute *attr;
    GtkWidget *w;

    info = &fields[i];
    if (info->unique) {
      /* Fast path unique fields, no need to search the entire contact */
      attr = e_vcard_get_attribute (E_VCARD (pane->priv->contact), info->vcard_field);
      if (!attr && pane->priv->editable) {
         attr = e_vcard_attribute_new ("", info->vcard_field);
         e_vcard_add_attribute (E_VCARD (pane->priv->contact), attr);
      }
      if (attr) {
        w = make_widget (pane, attr, info, size);
        gtk_box_pack_start (GTK_BOX (pane), w, FALSE, FALSE, 4);
      }
    } else {
      GList *attrs, *l;
      attrs = contact_get_attributes (pane->priv->contact, info->vcard_field);
      if (g_list_length (attrs) < 1 && pane->priv->editable) {
        attr = e_vcard_attribute_new ("", info->vcard_field);
        e_vcard_add_attribute (E_VCARD (pane->priv->contact), attr);
        w = make_widget (pane, attr, info, size);
        gtk_box_pack_start (GTK_BOX (pane), w, FALSE, FALSE, 4);
      }
      else
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

    /* strip empty attributes */
    if (editable == FALSE && pane->priv->contact)
      strip_empty_attributes (E_VCARD (pane->priv->contact));

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
  priv = pane->priv;

  /* check to see if the contact is the same as the current one */
  if (priv->contact && contact) {
    if (strcmp (e_contact_get_const (contact, E_CONTACT_UID),
                e_contact_get_const (priv->contact, E_CONTACT_UID)) == 0) {
      return;
    }
  }

  if (priv->contact) {
    strip_empty_attributes (E_VCARD (priv->contact));
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
