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

#include "contacts-history.h"

#include <string.h>
#include <time.h>
#include <libebook/e-book.h>
#include <moko-journal.h>
#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>
#include <libjana-gtk/jana-gtk.h>
#include <libical/ical.h>

#include "hito-vcard-util.h"

G_DEFINE_TYPE (ContactsHistory, contacts_history, GTK_TYPE_TREE_VIEW);

#define CONTACTS_HISTORY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
	CONTACTS_TYPE_HISTORY, \
	ContactsHistoryPrivate))

struct _ContactsHistoryPrivate 
{
  MokoJournal       *journal;
  JanaStore         *sms_store;
  JanaStoreView     *sms_store_view;
  gboolean          sms_store_open;
  
  GtkTreeModel      *list_store;
  GtkTreeModel      *filter;
  GHashTable        *iter_index;
  GHashTable        *category_colours;
  
  GdkPixbuf         *call_in_icon;
  GdkPixbuf         *call_out_icon;
  GdkPixbuf         *call_missed_icon;
  GdkPixbuf         *sms_icon;
  GdkPixbuf         *email_icon;
  
  EContact          *contact;
  
  GtkTreePath       *old_path;
};

static gchar *call_in_category[] = { "call-in", NULL };
static gchar *call_out_category[] = { "call-out", NULL };
static gchar *call_missed_category[] = { "call-missed", NULL };
static gchar *sms_category[] = { "sms", NULL };
static gchar *email_category[] = { "email", NULL };

enum
{
  PROP_0,
  PROP_CONTACT
};

enum
{
  CAT_CALL_IN,
  CAT_CALL_OUT,
  CAT_CALL_MISSED,
  CAT_EMAIL,
  CAT_SMS
};

enum
{
  COL_CATEGORY,
  COL_NAME,
  COL_TIME,
  COL_DETAIL,
  COL_ENTRY,
  COL_EXPAND,
  COL_LAST,
};

static GtkTreeViewClass *parent_class = NULL;

#define HISTORY_EMAIL_ICON        "moko-history-mail"
#define HISTORY_SMS_ICON          "moko-history-im"
#define HISTORY_CALL_IN_ICON      "moko-history-call-in"
#define HISTORY_CALL_OUT_ICON     "moko-history-call-out"
#define HISTORY_CALL_MISSED_ICON  "moko-history-call-missed"

void
contacts_history_update_uid (ContactsHistory *history, EContact *contact)
{
  g_return_if_fail (CONTACTS_IS_HISTORY (history));
  
  g_object_set (G_OBJECT (history), "contact", contact, NULL);
}

static void
add_voice_entry (MokoJournal *journal, MokoJournalEntry *entry,
                 ContactsHistory *history)
{
  gchar *detail;
  gboolean missed;
  const gchar *uid;
  GtkTreeIter *iter;
  gint hours, minutes, lminutes;
  glong lseconds;
  const MokoTime *mtime;
  MessageDirection direction;
  ContactsHistoryPrivate *priv;

  JanaTime *time = NULL;

  priv = CONTACTS_HISTORY_GET_PRIVATE (history);

  uid = moko_journal_entry_get_uid (entry);
  
  hours = minutes = lminutes = lseconds = 0;
  
  /* Get call start */
  mtime = moko_journal_entry_get_dtstart (entry);
  if (mtime) {
    const MokoTime *dtend;
    struct icaltimetype icaltime =
      icaltime_from_timet (moko_time_as_timet (mtime), 0);
    
    time = jana_ecal_time_new_from_icaltime (&icaltime);
    hours = jana_time_get_hours (time);
    minutes = jana_time_get_minutes (time);
    
    /* Get call length */
    dtend = moko_journal_entry_get_dtend (entry);
    if (dtend) {
      JanaTime *end_time;

      icaltime =
        icaltime_from_timet (moko_time_as_timet (mtime), 0);
      
      end_time = jana_ecal_time_new_from_icaltime (&icaltime);
      jana_utils_time_diff (time, end_time, NULL, NULL, NULL,
                            NULL, &lminutes, &lseconds);
      g_object_unref (end_time);
    }
  }
  
  moko_journal_entry_get_direction (entry, &direction);
  missed = moko_journal_voice_info_get_was_missed (entry);
  detail = g_strdup_printf ("%d:%02d: %s Call (%02d:%02ld)",
                            hours, minutes,
                            missed ?
                              "Missed" : ((direction == DIRECTION_IN) ?
                                "Received" : "Dialled"),
                            lminutes, lseconds);

  iter = g_slice_new0 (GtkTreeIter);
  gtk_list_store_insert_with_values (GTK_LIST_STORE (priv->list_store), iter,
                                     0, COL_CATEGORY,
                                     missed ? CAT_CALL_MISSED :
                                       ((direction == DIRECTION_IN) ?
                                         CAT_CALL_IN : CAT_CALL_OUT),
                                     COL_NAME, detail, COL_TIME, time,
                                     COL_ENTRY, entry,
                                     /*COL_DETAIL, detail,*/ -1);
  
  g_free (detail);
  
  g_hash_table_insert (priv->iter_index, g_strdup (uid), iter);
}

static void
journal_entry_added_cb (MokoJournal *journal, MokoJournalEntry *entry,
                        ContactsHistory *history)
{
  switch (moko_journal_entry_get_entry_type (entry)) {
    case EMAIL_JOURNAL_ENTRY :
    case SMS_JOURNAL_ENTRY :
      break;
    case VOICE_JOURNAL_ENTRY :
      add_voice_entry (journal, entry, history);
      break;
    default :
      break;
  }
}

static void
journal_entry_removed_cb (MokoJournal *journal, MokoJournalEntry *entry,
                          ContactsHistory *history)
{
  const gchar *uid;
  GtkTreeIter *iter;
  ContactsHistoryPrivate *priv;

  priv = CONTACTS_HISTORY_GET_PRIVATE (history);

  uid = moko_journal_entry_get_uid (entry);
  if (!(iter = g_hash_table_lookup (priv->iter_index, uid))) return;
  
  gtk_list_store_remove (GTK_LIST_STORE (priv->list_store), iter);
  g_hash_table_remove (priv->iter_index, uid);
}

static gchar *
sms_get_type_string (JanaNote *note, JanaTime *time)
{
  gint hours, minutes;
  gchar **categories;
  gchar *string;

  if (time) {
    hours = jana_time_get_hours (time);
    minutes = jana_time_get_minutes (time);
  } else
    hours = minutes = 0;
  
  categories = jana_component_get_categories ((JanaComponent *)note);
  string = g_strdup_printf ("%d:%02d: %s text message",
                            hours, minutes,
                            jana_utils_component_has_category (
                              (JanaComponent *)note, "Sent") ?
                              "Sent" : "Received");
  g_strfreev (categories);
  
  return string;
}

static void
sms_added_cb (JanaStoreView *store_view, GList *components,
              ContactsHistory *history)
{
  ContactsHistoryPrivate *priv;

  priv = CONTACTS_HISTORY_GET_PRIVATE (history);
  
  for (; components; components = components->next) {
    gchar *uid, *name, *detail;
    GtkTreeIter *iter;
    JanaTime *time;
    JanaNote *note;
    
    if (!JANA_IS_NOTE (components->data)) continue;
    else note = (JanaNote *)components->data;
    
    detail = jana_note_get_body (note);
    time = jana_note_get_creation_time (note);
    name = sms_get_type_string (note, time);
    
    iter = g_slice_new0 (GtkTreeIter);
    gtk_list_store_insert_with_values (GTK_LIST_STORE (priv->list_store), iter,
                                       0, COL_CATEGORY, CAT_SMS,
                                       COL_NAME, name, COL_TIME, time,
                                       COL_DETAIL, detail, -1);
    
    g_free (name);
    g_free (detail);
    g_object_unref (time);
    
    uid = jana_component_get_uid ((JanaComponent *)note);
    g_hash_table_insert (priv->iter_index, uid, iter);
  }
}

static void
sms_modified_cb (JanaStoreView *store_view, GList *components,
                 ContactsHistory *history)
{
  ContactsHistoryPrivate *priv;

  priv = CONTACTS_HISTORY_GET_PRIVATE (history);
  
  for (; components; components = components->next) {
    gchar *uid, *name, *detail;
    GtkTreeIter *iter;
    JanaTime *time;
    JanaNote *note;
    
    if (!JANA_IS_NOTE (components->data)) continue;
    else note = (JanaNote *)components->data;
    
    uid = jana_component_get_uid ((JanaComponent *)note);
    if (!(iter = g_hash_table_lookup (priv->iter_index, uid))) {
      g_free (uid);
      continue;
    }

    detail = jana_note_get_body (note);
    time = jana_note_get_creation_time (note);
    name = sms_get_type_string (note, time);
    
    gtk_list_store_set (GTK_LIST_STORE (priv->list_store), iter,
                        COL_NAME, name, COL_TIME, time, COL_DETAIL, detail, -1);

    g_free (uid);
    g_free (name);
    g_free (detail);
    g_object_unref (time);
  }
}

static void
sms_removed_cb (JanaStoreView *store_view, GList *uids,
                ContactsHistory *history)
{
  ContactsHistoryPrivate *priv;

  priv = CONTACTS_HISTORY_GET_PRIVATE (history);
  
  for (; uids; uids = uids->next) {
    GtkTreeIter *iter;

    if (!(iter = g_hash_table_lookup (priv->iter_index, uids->data))) continue;
    
    gtk_list_store_remove (GTK_LIST_STORE (priv->list_store), iter);
    g_hash_table_remove (priv->iter_index, uids->data);
  }
}

static void
refresh_sms_query (ContactsHistory *history)
{
  ContactsHistoryPrivate *priv;
  GList *numbers, *n;
  
  priv = CONTACTS_HISTORY_GET_PRIVATE (history);
  
  if ((!priv->sms_store_open) || (!priv->contact)) return;
  
  if (!priv->sms_store_view) {
    priv->sms_store_view = jana_store_get_view (priv->sms_store);

    /* Connect to added/modified/removed signals */
    g_signal_connect (priv->sms_store_view, "added",
                      G_CALLBACK (sms_added_cb), history);
    g_signal_connect (priv->sms_store_view, "modified",
                      G_CALLBACK (sms_modified_cb), history);
    g_signal_connect (priv->sms_store_view, "removed",
                      G_CALLBACK (sms_removed_cb), history);
    
    /* Start view */
    jana_store_view_start (priv->sms_store_view);
  }
  
  /* Query all notes with this number as author or recipient */
  /* TODO: Normalisation? */
  jana_store_view_clear_matches (priv->sms_store_view);
  numbers = hito_vcard_get_named_attributes (E_VCARD (priv->contact), EVC_TEL);
  if (numbers) {
    for (n = numbers; n; n = n->next) {
      jana_store_view_add_match (priv->sms_store_view, JANA_STORE_VIEW_AUTHOR,
                               hito_vcard_attribute_get_value_string (n->data));
      jana_store_view_add_match (priv->sms_store_view,JANA_STORE_VIEW_RECIPIENT,
                               hito_vcard_attribute_get_value_string (n->data));
    }
    g_list_free (numbers);
  } else {
    /* Nasty way of clearing the query, maybe libjana needs a more comprehensive
     * matching query API?
     */
    jana_store_view_add_match (priv->sms_store_view, JANA_STORE_VIEW_AUTHOR,
                               "nomatch");
  }
}

static void
sms_store_opened_cb (JanaStore *store, ContactsHistory *self)
{
  ContactsHistoryPrivate *priv;

  priv = CONTACTS_HISTORY_GET_PRIVATE (self);
  
  priv->sms_store_open = TRUE;
  refresh_sms_query (self);
}

static void
filter_func (GtkTreeModel *model, GtkTreeIter *iter, GValue *value,
             gint column, ContactsHistory *self)
{
  ContactsHistoryPrivate *priv;
  GtkTreeIter real_iter;
  gpointer pointer;

  priv = CONTACTS_HISTORY_GET_PRIVATE (self);
  
  gtk_tree_model_filter_convert_iter_to_child_iter ((GtkTreeModelFilter *)model,
                                                    &real_iter, iter);
  
  if (column < COL_LAST)
    gtk_tree_model_get (priv->list_store, &real_iter, column, &pointer, -1);
  switch (column) {
    case COL_CATEGORY :
      switch ((gint)pointer) {
        case CAT_CALL_IN :
          g_value_set_boxed (value, call_in_category);
          break;
        case CAT_CALL_OUT :
          g_value_set_boxed (value, call_out_category);
          break;
        case CAT_CALL_MISSED :
          g_value_set_boxed (value, call_missed_category);
          break;
        case CAT_EMAIL :
          g_value_set_boxed (value, email_category);
          break;
        case CAT_SMS :
          g_value_set_boxed (value, sms_category);
          break;
      }
      break;
    case COL_NAME :
    case COL_DETAIL :
      g_value_take_string (value, pointer);
      break;
    case COL_TIME :
      g_value_take_object (value, pointer);
      break;
    case COL_ENTRY :
      g_value_take_object (value, pointer);
      break;
    case COL_EXPAND :
      g_value_set_boolean (value, (gboolean)pointer);
      break;
    case COL_LAST :
      gtk_tree_model_get (priv->list_store, &real_iter,
                          COL_CATEGORY, &pointer, -1);
      switch ((gint)pointer) {
        case CAT_CALL_IN :
          g_value_set_object (value, priv->call_in_icon);
          break;
        case CAT_CALL_OUT :
          g_value_set_object (value, priv->call_out_icon);
          break;
        case CAT_CALL_MISSED :
          g_value_set_object (value, priv->call_missed_icon);
          break;
        case CAT_EMAIL :
          g_value_set_object (value, priv->email_icon);
          break;
        case CAT_SMS :
          g_value_set_object (value, priv->sms_icon);
          break;
      }
      break;
  }
}

static gboolean
visible_func (GtkTreeModel *model, GtkTreeIter *iter, ContactsHistory *history)
{
  const gchar *author, *recipient;
  ContactsHistoryPrivate *priv;
  MokoJournalEntry *entry;
  GList *numbers, *n;
  
  gboolean result = FALSE;

  priv = CONTACTS_HISTORY_GET_PRIVATE (history);
  
  if (!priv->contact) return FALSE;
  
  gtk_tree_model_get (model, iter, COL_ENTRY, &entry, -1);
  
  if (!entry) return TRUE;
  
  /* TODO: Handle non-voice journal entries? */
  
  author = moko_journal_voice_info_get_local_number (entry);
  recipient = moko_journal_voice_info_get_distant_number (entry);
  if ((!author) && (!recipient)) {
    moko_journal_entry_unref (entry);
    return FALSE;
  }
  
  /* Check if journal entry has the same number as author or recipient */
  /* TODO: Normalisation? */
  numbers = hito_vcard_get_named_attributes (E_VCARD (priv->contact), EVC_TEL);
  if (numbers) {
    for (n = numbers; n; n = n->next) {
      const gchar *number = hito_vcard_attribute_get_value_string (n->data);
      if ((author && (strcmp (number, author) == 0)) ||
          (recipient && (strcmp (number, recipient) == 0))) {
        result = TRUE;
        break;
      }
    }
    g_list_free (numbers);
  }
  
  moko_journal_entry_unref (entry);

  return result;
}

/* GObject functions */

static void
contacts_history_set_property (GObject      *object, 
			       guint         prop_id,
			       const GValue *value, 
			       GParamSpec   *pspec)
{
  g_return_if_fail(CONTACTS_IS_HISTORY(object));

  switch (prop_id) {
    case PROP_CONTACT:
      contacts_history_set_contact (CONTACTS_HISTORY (object),
                                    g_value_get_object (value));
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
contacts_history_get_property (GObject    *object, 
			       guint       prop_id,
			       GValue     *value, 
			       GParamSpec *pspec)
{
  ContactsHistoryPrivate *priv;
	
  g_return_if_fail(CONTACTS_IS_HISTORY(object));
  priv = CONTACTS_HISTORY_GET_PRIVATE (object);

  switch (prop_id) {
    case PROP_CONTACT:
      g_value_set_object (value, priv->contact);
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  } 
}


static void
contacts_history_finalize(GObject *obj)
{
  ContactsHistoryPrivate *priv;
	
  g_return_if_fail(CONTACTS_IS_HISTORY(obj));

  priv = CONTACTS_HISTORY_GET_PRIVATE (obj);
  
  if (priv->contact)
    g_object_unref (priv->contact);
  
  if (priv->journal)
    moko_journal_close (priv->journal);
  
  if (priv->sms_store_view)
    g_object_unref (priv->sms_store_view);
  
  if (priv->sms_store)
    g_object_unref (priv->sms_store);
  
  if (priv->iter_index)
    g_hash_table_unref (priv->iter_index);
  
  if (priv->category_colours)
    g_hash_table_unref (priv->category_colours);
  
  if (priv->call_in_icon)
    g_object_unref (priv->call_in_icon);
  
  if (priv->call_out_icon)
    g_object_unref (priv->call_out_icon);
  
  if (priv->call_missed_icon)
    g_object_unref (priv->call_missed_icon);
  
  if (priv->sms_icon)
    g_object_unref (priv->sms_icon);
  
  if (priv->email_icon)
    g_object_unref (priv->email_icon);
  
  if (priv->old_path)
    gtk_tree_path_free (priv->old_path);

  G_OBJECT_CLASS(contacts_history_parent_class)->finalize (obj);
}

static void
contacts_history_class_init (ContactsHistoryClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = GTK_TREE_VIEW_CLASS (klass);
  
  gobject_class->finalize = contacts_history_finalize;
  gobject_class->get_property = contacts_history_get_property;
  gobject_class->set_property = contacts_history_set_property;  
  
  g_type_class_add_private (gobject_class, sizeof (ContactsHistoryPrivate));
  
  /* Install class properties */
  g_object_class_install_property (gobject_class,
		                   PROP_CONTACT,
		                   g_param_spec_object ("contact",
		                     "EContact *",
		                     "The contact",
		                     E_TYPE_CONTACT,
		                     G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
}

static void
selection_changed_cb (GtkTreeSelection *selection, ContactsHistory *history)
{
  GtkTreeIter iter, real_iter;
  ContactsHistoryPrivate *priv;
  
  priv = CONTACTS_HISTORY_GET_PRIVATE (history);
  
  if (priv->old_path) {
    if (gtk_tree_model_get_iter (priv->list_store, &iter, priv->old_path)) {
      gtk_list_store_set ((GtkListStore *)priv->list_store, &iter,
                          COL_EXPAND, FALSE, -1);
    }
    gtk_tree_path_free (priv->old_path);
    priv->old_path = NULL;
  }
  
  if ((!selection) ||
      (!gtk_tree_selection_get_selected (selection, NULL, &iter))) return;
  
  gtk_tree_model_filter_convert_iter_to_child_iter ((GtkTreeModelFilter *)
                                                    priv->filter,
                                                    &real_iter, &iter);
  gtk_list_store_set ((GtkListStore *)priv->list_store, &real_iter,
                      COL_EXPAND, TRUE, -1);
  priv->old_path = gtk_tree_model_get_path (priv->list_store, &real_iter);
}

static gint
time_sort_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
                ContactsHistory *history)
{
  JanaTime *time1, *time2;
  gint result;
  
  gtk_tree_model_get (model, a, COL_TIME, &time1, -1);
  gtk_tree_model_get (model, b, COL_TIME, &time2, -1);
  
  if (!time1) {
    if (time2) {
      g_object_unref (time2);
      return 1;
    } else
      return 0;
  } else if (!time2) {
    if (time1) {
      g_object_unref (time1);
      return -1;
    } else
      return 0;
  }
  
  result = jana_utils_time_compare (time1, time2, FALSE);
  
  g_object_unref (time1);
  g_object_unref (time2);
  
  return result;
}

static void
free_iter_slice (gpointer value)
{
	g_slice_free (GtkTreeIter, value);
}

static void
free_colour_slice (gpointer value)
{
	g_slice_free (GdkColor, value);
}

static void
contacts_history_init (ContactsHistory *self)
{
  ContactsHistoryPrivate *priv;
  GtkCellRenderer *renderer;
  gint width, height;
  GdkColor *call_in_colour, *call_out_colour, *call_missed_colour,
           *sms_colour, *email_colour;
	
  priv = CONTACTS_HISTORY_GET_PRIVATE (self);
  
  /* Load icons */
  /* TODO: This should really be done in style-set */
  gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, &width, &height);
  priv->call_in_icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                 HISTORY_CALL_IN_ICON,
                                                 width, 0, NULL);
  priv->call_out_icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                  HISTORY_CALL_OUT_ICON,
                                                  width, 0, NULL);
  priv->call_missed_icon =
                        gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                  HISTORY_CALL_MISSED_ICON,
                                                  width, 0, NULL);
  priv->sms_icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                             HISTORY_SMS_ICON,
                                             width, 0, NULL);
  priv->email_icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                               HISTORY_EMAIL_ICON,
                                               width, 0, NULL);
  
  /* Set tree-view style */
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self), FALSE);
  
  /* Create hash tables */
  priv->iter_index = g_hash_table_new_full (g_str_hash, g_str_equal,
                                            (GDestroyNotify)g_free,
                                            (GDestroyNotify)free_iter_slice);
  priv->category_colours = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                  NULL, (GDestroyNotify)
                                                    free_colour_slice);
  
  /* Add colours to hash table */
  /* TODO: These should probably be theme-based and set on style-set */
  call_in_colour = g_slice_new0 (GdkColor);
  gdk_color_parse ("#E7FFE6", call_in_colour);
  gdk_colormap_alloc_color (gtk_widget_get_colormap (GTK_WIDGET (self)),
                            call_in_colour, FALSE, TRUE);
  g_hash_table_insert (priv->category_colours, call_in_category[0],
                       call_in_colour);

  call_out_colour = g_slice_new0 (GdkColor);
  gdk_color_parse ("#FFF7E6", call_out_colour);
  gdk_colormap_alloc_color (gtk_widget_get_colormap (GTK_WIDGET (self)),
                            call_out_colour, FALSE, TRUE);
  g_hash_table_insert (priv->category_colours, call_out_category[0],
                       call_out_colour);

  call_missed_colour = g_slice_new0 (GdkColor);
  gdk_color_parse ("#FAD7D2", call_missed_colour);
  gdk_colormap_alloc_color (gtk_widget_get_colormap (GTK_WIDGET (self)),
                            call_missed_colour, FALSE, TRUE);
  g_hash_table_insert (priv->category_colours, call_missed_category[0],
                       call_missed_colour);

  email_colour = g_slice_new0 (GdkColor);
  gdk_color_parse ("#FFF", email_colour);
  gdk_colormap_alloc_color (gtk_widget_get_colormap (GTK_WIDGET (self)),
                            email_colour, FALSE, TRUE);
  g_hash_table_insert (priv->category_colours, email_category[0],
                       email_colour);

  sms_colour = g_slice_new0 (GdkColor);
  gdk_color_parse ("#E6F9FF", sms_colour);
  gdk_colormap_alloc_color (gtk_widget_get_colormap (GTK_WIDGET (self)),
                            sms_colour, FALSE, TRUE);
  g_hash_table_insert (priv->category_colours, sms_category[0],
                       sms_colour);
  
  /* Create model and filter */
  priv->list_store = (GtkTreeModel *)gtk_list_store_new (COL_LAST, G_TYPE_INT,
                                                         G_TYPE_STRING,
                                                         G_TYPE_OBJECT,
                                                         G_TYPE_STRING,
                                                        MOKO_TYPE_JOURNAL_ENTRY,
                                                         G_TYPE_BOOLEAN);
  priv->filter = gtk_tree_model_filter_new (priv->list_store, NULL);
  g_object_unref (priv->list_store);
  gtk_tree_model_filter_set_modify_func ((GtkTreeModelFilter *)
                                         priv->filter, COL_LAST + 1,
                                         (GType []){G_TYPE_STRV, G_TYPE_STRING,
                                                    G_TYPE_OBJECT,
                                                    G_TYPE_STRING,
                                                    MOKO_TYPE_JOURNAL_ENTRY,
                                                    G_TYPE_BOOLEAN,
                                                    GDK_TYPE_PIXBUF },
                                         (GtkTreeModelFilterModifyFunc)
                                          filter_func, self, NULL);
  gtk_tree_model_filter_set_visible_func ((GtkTreeModelFilter *)
                                          priv->filter,
                                          (GtkTreeModelFilterVisibleFunc)
                                          visible_func, self, NULL);
  gtk_tree_view_set_model (GTK_TREE_VIEW (self), priv->filter);
  g_object_unref (priv->filter);
  
  /* Set sort function */
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->list_store),
                                   COL_TIME, (GtkTreeIterCompareFunc)
                                   time_sort_func, self, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->list_store),
                                        COL_TIME, GTK_SORT_DESCENDING);
  
  /* Create cell renderer and insert column */
  renderer = jana_gtk_cell_renderer_note_new ();
  g_object_set (G_OBJECT (renderer), "show_recipient", FALSE,
                "draw-box", TRUE, "xpad", 6, "ypad", 6,
                "xpad-inner", 6, "ypad-inner", 6,
                "category-color-hash", priv->category_colours, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (self), 0, NULL,
                                               renderer, "author", COL_NAME,
                                               "body", COL_DETAIL, "created",
                                               COL_TIME, "icon", COL_LAST,
                                               "show-body", COL_EXPAND,
                                               "categories", COL_CATEGORY,
                                               NULL);
  
  /* Hook onto selection changed to expand the currently selected item */
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (self)),
                    "changed", G_CALLBACK (selection_changed_cb), self);
  
  /* Create the MokoJournal object */
  priv->journal = moko_journal_open_default ();
  if (priv->journal) {
    /* Connect to added/removed signals */
    g_signal_connect (priv->journal, "entry_added",
                      G_CALLBACK (journal_entry_added_cb), self);
    g_signal_connect (priv->journal, "entry_removed",
                      G_CALLBACK (journal_entry_removed_cb), self);
    
    /* load all journal entries from the journal on storage*/
    if (!moko_journal_load_from_storage (priv->journal))
      g_warning ("Unable to load journal from storage");
  } else {
    g_warning ("Could not open the journal");
  }
  
  /* Create the SMS store and open */
  priv->sms_store = jana_ecal_store_new (JANA_COMPONENT_NOTE);
  if (priv->sms_store) {
    g_signal_connect (priv->sms_store, "opened",
                      G_CALLBACK (sms_store_opened_cb), self);
    jana_store_open (priv->sms_store);
  } else {
    g_warning ("Could not retrieve SMS store");
  }
}

GtkWidget*
contacts_history_new (void)
{
  ContactsHistory *history;
  
  history = g_object_new (CONTACTS_TYPE_HISTORY, NULL);
        
  return GTK_WIDGET(history);
}

void
contacts_history_set_contact (ContactsHistory *history, EContact *contact)
{
  ContactsHistoryPrivate *priv;
	
  priv = CONTACTS_HISTORY_GET_PRIVATE (history);

  if (priv->contact) {
    g_object_unref (priv->contact);
    priv->contact = NULL;
  }
  
  if (contact) {
    priv->contact = g_object_ref (contact);
    refresh_sms_query (history);
  }
  
  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
}

