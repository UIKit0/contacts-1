
#include <glib.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>

char *e_util_unicode_get_utf8 (const char *text, gunichar * out);

const char *kozo_utf8_strstrcasestrip (const char *haystack,
				       const gunichar * needle);

gunichar *kozo_utf8_strcasestrip (const char *str);

EContact *get_contact_from_selection (GtkTreeSelection *selection);

EContact *get_current_contact ();

void contact_selected_sensitive (gboolean sensitive);

GtkImage *contacts_load_photo (EContact *contact);

void contacts_clean_contact (EContact *contact);

gchar *contacts_string_list_as_string (GList *list,
					      const gchar *separator);

GList *contacts_get_string_list_from_types (GList *params);

void contacts_choose_photo (GtkWidget *button, EContact *contact);

void contacts_free_list_hash (gpointer data);
