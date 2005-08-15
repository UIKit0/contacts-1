
#include <glib.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>

char *e_util_unicode_get_utf8 (const char *text, gunichar * out);

gunichar stripped_char (gunichar ch);

const char *kozo_utf8_strstrcasestrip (const char *haystack,
				       const gunichar * needle);

gunichar *kozo_utf8_strcasestrip (const char *str);

void free_object_list (GList * list);

EContact *get_contact_from_selection (GtkTreeSelection *selection);

EContact *get_current_contact ();

void contact_selected_sensitive (gboolean sensitive);

GtkImage *load_contact_photo (EContact *contact);
