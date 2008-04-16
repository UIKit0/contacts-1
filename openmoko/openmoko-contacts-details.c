/*
 * Copyright (C) 2007 OpenMoko, Inc
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <config.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>
#include <string.h>
#include <moko-finger-scroll.h>
#include <moko-stock.h>
#include <moko-hint-entry.h>

#include "openmoko-contacts.h"
#include "contacts-attribute-store.h"
#include "contacts-attributes-editor.h"
#include "openmoko-contacts-details.h"

#include "hito-contact-preview.h"
#include "hito-vcard-util.h"
#include "koto-cell-renderer-pixbuf.h"

#if GLIB_MINOR_VERSION < 12
#include "gbase64.h"
#endif


struct _AttributeName {
  gchar *vcard_name;
  gchar *pretty_name;
};

static const struct _AttributeName attr_names[] = {
  {EVC_TEL, "Telephone"},
  {EVC_EMAIL, "E-Mail"},
  {EVC_FN, "Fullname"},
  {EVC_ORG, "Organization"}
};

static void edit_toggle_toggled_cb (GtkWidget *button, ContactsData *data);
static void delete_contact_clicked_cb (GtkWidget *button, ContactsData *data);
static void fullname_changed_cb (GtkWidget *entry, ContactsData *data);
static void org_changed_cb (GtkWidget *entry, ContactsData *data);
static void attribute_changed (const gchar *attr_name, const gchar *new_val, ContactsData *data);



static gboolean
contacts_details_page_map_event_cb (GtkWidget *widget, ContactsData *data)
{
  static gboolean mapped = FALSE;

  /* make sure the UI is set up properly when initially mapped */
  if (!mapped)
  {
    edit_toggle_toggled_cb (NULL, data);
    mapped = TRUE;
  }
  return FALSE;
}

void
mark_contact_dirty (ContactsData *data)
{
  if (!data->detail_page_loading)
  {
    data->dirty = TRUE;
  }
}

static gboolean
address_field_empty (GtkTextView *text_view)
{
  GtkTextBuffer *buf;
  GtkTextIter start, end;
  gchar *text;

  buf = gtk_text_view_get_buffer (text_view);

  if (gtk_text_buffer_get_char_count (buf) == 0)
    return TRUE;

  gtk_text_buffer_get_end_iter (buf, &end);
  gtk_text_buffer_get_start_iter (buf, &start);

  text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

  if (text && !strcmp (text, "Address"))
  {
    g_free (text);
    return TRUE;
  }

  return FALSE;
}

static void
address_buffer_changed_cb (GtkTextBuffer *buf, ContactsData *data)
{
  gchar *p, *value = NULL;
  GtkTextIter start, end;

 
  if (address_field_empty (GTK_TEXT_VIEW (data->address)))
    return;

  gtk_text_buffer_get_end_iter (buf, &end);
  gtk_text_buffer_get_start_iter (buf, &start);

  value = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

  p = value;
  while (*p)
  {
    if (*p == '\n')
      *p = ';';
    p++;
  }
  attribute_changed (EVC_ADR, value, data);
  g_free (value);
}

static gboolean
address_focus_out_cb (GtkTextView *text_view, GdkEventFocus *event, gpointer user_data)
{
  GtkTextBuffer *buf;
  gint i;
  
  buf = gtk_text_view_get_buffer (text_view);
  i = gtk_text_buffer_get_char_count (buf);

  if (i == 0)
  {
    gtk_widget_modify_text (GTK_WIDGET (text_view), GTK_STATE_NORMAL,
        &GTK_WIDGET (text_view)->style->text[GTK_STATE_INSENSITIVE]);
    gtk_text_buffer_set_text (buf, "Address", -1);
  }


  return FALSE;
}

static gboolean
address_focus_in_cb (GtkTextView *text_view, GdkEventFocus *event, gpointer user_data)
{
  GtkTextBuffer *buf;
  GtkTextIter start, end;
  gchar *text;

  buf = gtk_text_view_get_buffer (text_view);

  gtk_text_buffer_get_end_iter (buf, &end);
  gtk_text_buffer_get_start_iter (buf, &start);
  
  text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
  
  if (text && !strcmp (text, "Address"))
  {
    gtk_text_buffer_set_text (buf, "", -1);
    gtk_widget_modify_text (GTK_WIDGET (text_view), GTK_STATE_NORMAL, NULL);
  }

  
  return FALSE;
}

static void
address_field_set_text (GtkTextView *text_view, gchar *value)
{
  GtkTextBuffer *buf;

  buf = gtk_text_view_get_buffer (text_view);

  gtk_text_buffer_set_text (buf, value, -1);

  address_focus_out_cb (text_view, NULL, NULL);
}



static gboolean
address_frame_expose_cb (GtkWidget *w, GdkEventExpose *e, gpointer user_data)
{
  gint width, height;
  gdk_window_set_background (e->window, &w->style->base[GTK_STATE_NORMAL]);
  gdk_drawable_get_size (e->window, &width, & height);
  gtk_paint_flat_box (w->style, w->window, GTK_WIDGET_STATE (w),
      GTK_SHADOW_NONE, &e->area, w, "", 0, 0, width, height);
  gtk_paint_shadow (w->style, w->window, GTK_WIDGET_STATE (w),
      GTK_SHADOW_IN, &e->area, w, "", 0, 0, width, height);
  return TRUE;
}


static void
address_frame_style_set_cb (GtkWidget *entry, GtkStyle *previous_style, GtkWidget *alignment)
{
  gboolean interior_focus;
  gint focus_width;
  gint xpad, ypad;

  /* we need to "borrow" the correct style settings from another GtkEntry */
  xpad = entry->style->xthickness;
  ypad = entry->style->ythickness;

  gtk_widget_style_get (entry,
      "interior-focus", &interior_focus,
      "focus-line-width", &focus_width, NULL);

  if (interior_focus)
  {
    xpad += focus_width;
    ypad += focus_width;
  }

  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), ypad, ypad, xpad, xpad);
  gtk_widget_modify_bg (alignment, GTK_STATE_NORMAL, entry->style->base);
}

void
create_contacts_details_page (ContactsData *data)
{

  GtkWidget *box, *hbox, *toolbar, *w, *sw, *viewport, *main_vbox;
  GtkToolItem *toolitem;
  GtkTreeModel *liststore;
  GtkTextBuffer *buffer;

  box = gtk_vbox_new (FALSE, 0);

  /* do further widget setup once they have been mapped
   * (some setup requires the style to have been set) */
  g_signal_connect (box, "map", G_CALLBACK (contacts_details_page_map_event_cb), data);

  contacts_notebook_add_page_with_icon (data->notebook, box, GTK_STOCK_FILE);

  toolbar = gtk_toolbar_new ();
  gtk_box_pack_start (GTK_BOX (box), toolbar, FALSE, FALSE, 0);

  toolitem = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_EDIT);
  g_signal_connect (G_OBJECT (toolitem), "toggled", G_CALLBACK (edit_toggle_toggled_cb), data);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
  data->edit_toggle = toolitem;

  toolitem = gtk_tool_button_new_from_stock (GTK_STOCK_DELETE);
  g_signal_connect (G_OBJECT (toolitem), "clicked", G_CALLBACK (delete_contact_clicked_cb), data);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);


  main_vbox = gtk_vbox_new (FALSE, PADDING);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), PADDING);

  sw = moko_finger_scroll_new ();
  gtk_box_pack_start (GTK_BOX (box), sw, TRUE, TRUE, PADDING);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (sw), viewport);
  gtk_container_add (GTK_CONTAINER (viewport), main_vbox);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);


  hbox = gtk_hbox_new (FALSE, PADDING);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  data->photo = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (hbox), data->photo, FALSE, FALSE, 0);

  w = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, PADDING);

  data->fullname = moko_hint_entry_new ("Name");
  gtk_box_pack_start (GTK_BOX (w), data->fullname, TRUE, TRUE, 0);
  g_signal_connect (data->fullname, "changed", G_CALLBACK (fullname_changed_cb), data);

  data->org = moko_hint_entry_new ("Organization");
  gtk_box_pack_start (GTK_BOX (w), data->org, TRUE, TRUE, 0);
  g_signal_connect (data->org, "changed", G_CALLBACK (org_changed_cb), data);


  /* liststore for attributes */
  liststore = contacts_attribute_store_new ();
  data->attribute_liststore = GTK_LIST_STORE (liststore);
  g_signal_connect_swapped (liststore, "row-changed", G_CALLBACK (mark_contact_dirty), data);
  g_signal_connect_swapped (liststore, "row-deleted", G_CALLBACK (mark_contact_dirty), data);
  g_signal_connect_swapped (liststore, "row-inserted", G_CALLBACK (mark_contact_dirty), data);

  /* telephone entries */
  data->telephone = contacts_attributes_editor_new (CONTACTS_ATTRIBUTE_STORE (liststore), EVC_TEL);
  gtk_box_pack_start (GTK_BOX (main_vbox), data->telephone, FALSE, FALSE, PADDING);

  /* email entries */
  data->email = contacts_attributes_editor_new (CONTACTS_ATTRIBUTE_STORE (liststore), EVC_EMAIL);
  gtk_box_pack_start (GTK_BOX (main_vbox), data->email, FALSE, FALSE, PADDING);

  /* add address field */

  /* here we try to simulate a GtkEntry
   * to get the correct padding between the shadow and the text view, we use a
   * GtkAlignment to set the padding, and borrow the thickness values from the
   * style of another GtkEntry */

  /* we use a GtkEventBox to draw the shadow and set the correct background color */
  sw = gtk_event_box_new ();
  g_signal_connect (sw, "expose-event", G_CALLBACK (address_frame_expose_cb), NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), sw, FALSE, FALSE, PADDING);
  gtk_widget_modify_bg (sw, GTK_STATE_NORMAL, sw->style->base);

  /* we need to add some padding between frame and textview */
  w = gtk_alignment_new (0, 0, 1, 1);
  gtk_container_add (GTK_CONTAINER (sw), w);
  /* when the style info for a GtkEntry is available, use it to set padding values */
  g_signal_connect (data->fullname, "style-set", G_CALLBACK (address_frame_style_set_cb), w);

  data->address = gtk_text_view_new ();
  gtk_container_add (GTK_CONTAINER (w), data->address);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (data->address));
  g_signal_connect (buffer, "changed", G_CALLBACK (address_buffer_changed_cb), data);
  g_signal_connect (data->address, "focus-in-event", G_CALLBACK (address_focus_in_cb), data);
  g_signal_connect (data->address, "focus-out-event", G_CALLBACK (address_focus_out_cb), data);

}

void
free_contacts_details_page (ContactsData *data)
{

  /* unref the attribute list */
  g_object_unref (data->attribute_liststore);

}


void
contacts_details_page_set_editable (ContactsData *data, gboolean editing)
{
  gboolean tb_state;

  tb_state = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (data->edit_toggle));

  /* change active state only if current state is different to new state */
  if (tb_state ^ editing)
  {
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (data->edit_toggle), editing);
  }
}

static void
contact_photo_size (GdkPixbufLoader * loader, gint width, gint height,
        gpointer user_data)
{
  /* Max height of GTK_ICON_SIZE_DIALOG */
  gint iconwidth, iconheight;
  gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &iconwidth, &iconheight);
  gdk_pixbuf_loader_set_size (loader, width / ((gdouble) height / iconheight), iconheight);
}


void
contacts_details_page_update (ContactsData *data)
{
  GtkListStore *liststore;
  const gchar *value;
  GList *list = NULL;

  liststore = data->attribute_liststore;

  data->detail_page_loading = TRUE;

  /* ensure the default mode is view, not edit */
  contacts_details_page_set_editable (data, FALSE);

  contacts_attribute_store_set_vcard (CONTACTS_ATTRIBUTE_STORE (liststore),
                                      E_VCARD (data->contact));
  if (!data->contact)
  {
    moko_hint_entry_set_text (MOKO_HINT_ENTRY (data->fullname), "");
    moko_hint_entry_set_text (MOKO_HINT_ENTRY (data->org), "");
    gtk_image_set_from_icon_name (GTK_IMAGE (data->photo), "stock_person", GTK_ICON_SIZE_DIALOG);
    address_field_set_text (GTK_TEXT_VIEW (data->address), "");
    return;
  }

  value = e_contact_get_const (data->contact, E_CONTACT_FULL_NAME);
  if (!value) value = "";
  moko_hint_entry_set_text (MOKO_HINT_ENTRY (data->fullname), value);
  
  value = e_contact_get_const (data->contact, E_CONTACT_ORG);
  if (!value) value = "";
  moko_hint_entry_set_text (MOKO_HINT_ENTRY (data->org), value);

  list = hito_vcard_get_named_attributes (E_VCARD (data->contact), EVC_ADR);
  if (list)
  {
    gchar *s, *p;

    p = s = e_vcard_attribute_get_value (list->data);

    while (*p)
    {
      if (*p == ';')
        *p = '\n';
      p++;
    }
    address_field_set_text (GTK_TEXT_VIEW (data->address), s);
    g_free (s);
    g_list_free (list);
  }

  list = hito_vcard_get_named_attributes (E_VCARD (data->contact), EVC_PHOTO);
  if (list)
  {
    GdkPixbufLoader *ploader;
    guchar *buf;
    gsize size;
    value = list->data;
    buf = g_base64_decode (value, &size);
    ploader = gdk_pixbuf_loader_new ();
    g_signal_connect (G_OBJECT (ploader), "size-prepared", G_CALLBACK (contact_photo_size),  NULL);

    gdk_pixbuf_loader_write (ploader, buf, size, NULL);
    gdk_pixbuf_loader_close (ploader, NULL);
    gtk_image_set_from_pixbuf (GTK_IMAGE (data->photo), g_object_ref (gdk_pixbuf_loader_get_pixbuf (ploader)));
    g_object_unref (ploader);
    g_list_free (list);
  }
  else
  {
    gtk_image_set_from_icon_name (GTK_IMAGE (data->photo), "stock_person", GTK_ICON_SIZE_DIALOG);
  }


  data->detail_page_loading = FALSE;

  /* ensure the UI is in a sane state */
  edit_toggle_toggled_cb (NULL, data);
}



static void
edit_toggle_toggled_cb (GtkWidget *button, ContactsData *data)
{
  gboolean editing;

  editing =
    gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (data->edit_toggle));

  contacts_attributes_editor_set_editable (CONTACTS_ATTRIBUTES_EDITOR (data->email), editing);
  contacts_attributes_editor_set_editable (CONTACTS_ATTRIBUTES_EDITOR (data->telephone), editing);

  gtk_entry_set_has_frame (GTK_ENTRY (data->fullname), editing);
  g_object_set (G_OBJECT (data->fullname), "editable", editing, NULL);

  gtk_entry_set_has_frame (GTK_ENTRY (data->org), editing);
  g_object_set (G_OBJECT (data->org), "editable", editing, NULL);

  g_object_set (G_OBJECT (data->address), "editable", editing, NULL);

  if (editing)
  {
    /* only reset the text colours if the field is not empty, but always reset
     * the base colour */
    if (!moko_hint_entry_is_empty (MOKO_HINT_ENTRY (data->fullname)))
      gtk_widget_modify_text (data->fullname, GTK_STATE_NORMAL, NULL);
    gtk_widget_modify_base (data->fullname, GTK_STATE_NORMAL, NULL);

    if (!moko_hint_entry_is_empty (MOKO_HINT_ENTRY (data->org)))
      gtk_widget_modify_text (data->org, GTK_STATE_NORMAL, NULL);
    gtk_widget_modify_base (data->org, GTK_STATE_NORMAL, NULL);

    /* make sure fullname, org and address are visible */
    gtk_widget_show (data->fullname);
    gtk_widget_show (data->org);

    /* parent of the address textview is the frame */
    gtk_widget_show (data->address->parent->parent);

  }
  else
  {
    if (moko_hint_entry_is_empty (MOKO_HINT_ENTRY (data->fullname)))
    {
      gtk_widget_hide (data->fullname);
    }
    else
    {
      gtk_widget_modify_text (data->fullname, GTK_STATE_NORMAL, &data->org->style->fg[GTK_STATE_NORMAL]);
      gtk_widget_modify_base (data->fullname, GTK_STATE_NORMAL, &data->org->style->bg[GTK_STATE_NORMAL]);
      gtk_widget_show (data->fullname);
    }

    if (moko_hint_entry_is_empty (MOKO_HINT_ENTRY (data->org)))
      gtk_widget_hide (data->org);
    else
    {
      gtk_widget_modify_text (data->org, GTK_STATE_NORMAL, &data->org->style->fg[GTK_STATE_NORMAL]);
      gtk_widget_modify_base (data->org, GTK_STATE_NORMAL, &data->org->style->bg[GTK_STATE_NORMAL]);
      gtk_widget_show (data->org);
    }

    if (address_field_empty (GTK_TEXT_VIEW (data->address)))
    {
      /* parent of the address textview is the frame */
      gtk_widget_hide (data->address->parent->parent);
    }
    else
    {
      gtk_widget_modify_text (data->address, GTK_STATE_NORMAL, &data->org->style->fg[GTK_STATE_NORMAL]);
      gtk_widget_show (data->address->parent->parent);
    }

    /* remove current focus to close any active edits */
    gtk_window_set_focus (GTK_WINDOW (data->window), NULL);

  }

}


static void
delete_contact_clicked_cb (GtkWidget *button, ContactsData *data)
{
  EContact *card;
  GtkWidget *dialog;
  const gchar *name = NULL;

  card = data->contact;
  name = e_contact_get_const (card, E_CONTACT_FULL_NAME);

  dialog = gtk_message_dialog_new (GTK_WINDOW (data->window),
      GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
      "Are you sure you want to remove \"%s\" from your address book?", name);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_DELETE, GTK_RESPONSE_OK, NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    GError *err = NULL;
    GtkWidget *err_message;
    const gchar *uid;
    uid = e_contact_get_const (card, E_CONTACT_UID);
    if (uid)
    {
      e_book_remove_contact (data->book, uid, &err);
    }
    gtk_widget_destroy (dialog);
    if (err)
    {
      err_message = gtk_message_dialog_new (GTK_WINDOW (data->window), 
          GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
          "The following error occured while attempting to delete the contact:\n\"%s\"", err->message);
      gtk_dialog_run (GTK_DIALOG (err_message));
      gtk_widget_destroy (err_message);
    }
    else
    {
      data->dirty = FALSE;

      /* return to contact list */
      gtk_notebook_set_current_page (GTK_NOTEBOOK (data->notebook), LIST_PAGE_NUM);
    }
  }
  else
  {
    gtk_widget_destroy (dialog);
  }

}

static void
attribute_changed (const gchar *attr_name, const gchar *new_val, ContactsData *data)
{
  EVCard *card;
  EVCardAttribute *attr;

  /* don't set the attribute if we are still loading it */
  if (data->detail_page_loading)
    return;

  card = E_VCARD (data->contact);
  attr = e_vcard_get_attribute (card, attr_name);

  if (!attr)
  {
    attr = e_vcard_attribute_new (NULL, attr_name);
    e_vcard_add_attribute (card, attr);
  }

  e_vcard_attribute_remove_values (attr);

  /* FIXME: this is not dealing with multi values yet */
  e_vcard_attribute_add_value (attr, new_val);

  data->dirty = TRUE;
}

static void
fullname_changed_cb (GtkWidget *entry, ContactsData *data)
{
  const gchar *new_val;

  new_val = gtk_entry_get_text (GTK_ENTRY (entry));

  if (moko_hint_entry_is_empty (MOKO_HINT_ENTRY (entry)))
    return;

  attribute_changed (EVC_FN, new_val, data);
}

static void
org_changed_cb (GtkWidget *entry, ContactsData *data)
{
  const gchar *new_val;

  new_val = gtk_entry_get_text (GTK_ENTRY (entry));

  if (moko_hint_entry_is_empty (MOKO_HINT_ENTRY (entry)))
    return;

  attribute_changed (EVC_ORG, new_val, data);
}


