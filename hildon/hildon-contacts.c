/*
 * Copyright (C) 2007 OpenedHand Ltd
 *
 * Hildon port by: Rob Bradford <rob@o-hand.com>
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
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>

#include <hildon/hildon-program.h>

#include "hito-contact-store.h"
#include "hito-contact-model-filter.h"
#include "hito-contact-preview.h"
#include "hito-contact-view.h"
#include "hito-group-store.h"
#include "hito-group-combo.h"
#include "hito-all-group.h"
#include "hito-category-group.h"
#include "hito-no-category-group.h"
#include "hito-separator-group.h"
#include "hito-vcard-util.h"

typedef struct
{
  HildonProgram *program;
  EBook *book;
  EBookView *book_view;

  GtkTreeModel *contact_store;
  GtkTreeSelection *selection;

  GtkWidget *main_window;
  GtkWidget *tv;
  GtkWidget *scrolled_window;
  GtkWidget *hpaned;
  GtkWidget *preview;
  GtkWidget *preview_box;
} ContactsApp;

static void
book_view_callback (EBook *book, EBookStatus status, EBookView *book_view,
    gpointer userdata)
{
  ContactsApp *app = (ContactsApp *)userdata;

  if (status == E_BOOK_ERROR_OK)
  {
    app->book_view = book_view;

    if (app->contact_store)
    {
      g_object_set (app->contact_store, "view", app->book_view, NULL);
    }

    /* Now start the view */
    e_book_view_start (app->book_view);
  } else {
    g_warning ("Error %d whilst getting book view asynchronously", status);
  }
}

static void
book_opened_cb (EBook *book, EBookStatus status, gpointer userdata)
{
  ContactsApp *app = (ContactsApp *)userdata;

  if (status == E_BOOK_ERROR_OK)
  {
    EBookQuery *query = NULL;

    query = e_book_query_any_field_contains (NULL);

    /*
     * Get the view with the query
     *
     * This function returns FALSE on success.
     */
    if (e_book_async_get_book_view (app->book, query, NULL, 0,
          (EBookBookViewCallback)book_view_callback, app) == TRUE)
    {
      g_warning ("Error whilst asking to get book asynchronously");
    }

    e_book_query_unref (query);
  } else {
    g_warning ("Error %d whilst opening book asynchronously", status);
  }
}

static void
tv_selection_changed_cb (GtkTreeSelection *selection, gpointer userdata)
{
  ContactsApp *app = (ContactsApp *)userdata;
  GtkTreeModel *model = NULL;
  GtkTreeIter iter;
  EContact *contact = NULL;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
  {
    gtk_tree_model_get (model, &iter, COLUMN_CONTACT, &contact, -1);

    /* Set the preview based on this contact */
    hito_contact_preview_set_contact (HITO_CONTACT_PREVIEW (app->preview), 
        contact);
  }
}

static gboolean
ensure_first_selected_cb (gpointer userdata)
{
  ContactsApp *app = (ContactsApp *)userdata;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;

  if (!gtk_tree_selection_get_selected (app->selection, &model, &iter))
  {
    gtk_tree_model_get_iter_first (model, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (app->tv), path, NULL, FALSE);
    gtk_tree_path_free (path);
  }

  return FALSE;
}

static void
row_inserted_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
    gpointer userdata)
{
  ContactsApp *app = (ContactsApp *)userdata;

  /* try to ensure that something is always selected */
  if (!gtk_tree_selection_get_selected (app->selection, NULL, NULL))
  {
    g_idle_add (ensure_first_selected_cb, app);
  }
}

static void
row_deleted_cb (GtkTreeModel *model, GtkTreePath *path, gpointer userdata)
{
  ContactsApp *app = (ContactsApp *)userdata;
  GtkTreeIter iter;

  if (gtk_tree_path_prev (path))
  {
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (app->tv), path, NULL, FALSE);
  } else {
    if (gtk_tree_model_get_iter (model, &iter, path))
    {
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (app->tv), path, NULL, FALSE);
    } else {
      gtk_widget_hide (app->preview);
    }
  }
}

int
main (int argc, char **argv)
{
  ContactsApp *app;
  GError *error = NULL;
  GtkWidget *alignment;

#ifdef ENABLE_NLS
  /* Initialise i18n*/
  bindtextdomain (GETTEXT_PACKAGE, CONTACTS_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  /* Boring init */
  if (!g_thread_supported ())
    g_thread_init (NULL);
  gtk_init (&argc, &argv);

  app = g_new0 (ContactsApp, 1);

  /* 
   * Get addressbook. Do this nice and early. Without this life is a bit
   * pointless so die.
   */
  app->book = e_book_new_system_addressbook (&error);

  if (app->book == NULL)
  {
    g_error ("Problem getting address book: %s", error->message);
  }

  /* 
   * Open address book. Here 2nd argument allows the book to be created if it
   * does not exist. Interesting stuff happens in the callback.
   *
   * This function returns FALSE on success.
   */
  if (e_book_async_open (app->book, FALSE, book_opened_cb, app) == TRUE)
  {
    g_error ("Error when asking to open address book asynchronously");
  }

  /* Basic setup */
  app->program = hildon_program_get_instance ();
  app->main_window = hildon_window_new ();
  hildon_program_add_window (app->program, HILDON_WINDOW (app->main_window));

  gtk_window_set_default_icon_name ("contacts");
  g_signal_connect (app->main_window, "destroy", gtk_main_quit, NULL);

  /* 
   * Create the main model. The view *might* be available. This function accepts
   * NULL anyway.
   */
  app->contact_store = hito_contact_store_new (app->book_view);

  /* 
   * Hook up some callbacks for faffing with ensuring something useful is
   * selected
   */
  g_signal_connect (app->contact_store, "row-inserted",
                    G_CALLBACK (row_inserted_cb), app);
  g_signal_connect (app->contact_store, "row-deleted",
                    G_CALLBACK (row_deleted_cb), app);

  /* Create the main tree view using this model. */
  app->tv = hito_contact_view_new (HITO_CONTACT_STORE (app->contact_store), NULL);
  app->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (app->scrolled_window), app->tv);

  /* Get a selection on this tv and setup the changed callback */
  app->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (app->tv));
  g_signal_connect (app->selection, "changed", G_CALLBACK (tv_selection_changed_cb), app);

  /* Create the viewer box for the side bar */
  app->preview_box = gtk_vbox_new (FALSE, 6);
  app->preview = hito_contact_preview_new ();
  alignment = gtk_alignment_new (0, 0, 1, 1);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 6, 6, 6);
  gtk_container_add (GTK_CONTAINER (alignment), app->preview);
  gtk_box_pack_start (GTK_BOX (app->preview_box), alignment, TRUE, TRUE, 2);

  /* Create the hpaned that we use to separate the two halves */
  app->hpaned = gtk_hpaned_new ();
  gtk_paned_pack1 (GTK_PANED (app->hpaned), app->scrolled_window, TRUE, FALSE);
  gtk_paned_pack2 (GTK_PANED (app->hpaned), app->preview_box, TRUE, FALSE);

  gtk_container_add (GTK_CONTAINER (app->main_window), app->hpaned);
  gtk_widget_show_all (app->main_window);

  gtk_main ();

  return 0;
}
