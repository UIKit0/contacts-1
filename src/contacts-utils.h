
#include <glib.h>
#include <gtk/gtk.h>
#include <libebook/e-book.h>
#include <glade/glade.h>

char *e_util_unicode_get_utf8 (const char *text, gunichar * out);

const char *kozo_utf8_strstrcasestrip (const char *haystack,
				       const gunichar * needle);

gunichar *kozo_utf8_strcasestrip (const char *str);

EContact *contacts_contact_from_selection (GtkTreeSelection *selection,
					   GHashTable *contacts_table);

EContact *contacts_get_selected_contact (GladeXML *xml,
					 GHashTable *contacts_table);

void contacts_set_available_options (GladeXML *xml, gboolean new, gboolean open,
				     gboolean delete);

GtkImage *contacts_load_photo (EContact *contact);

void contacts_clean_contact (EContact *contact);

gchar *contacts_string_list_as_string (GList *list,
					      const gchar *separator);

GList *contacts_get_types (GList *params);

GList *contacts_get_type_strings (GList *params);

void contacts_choose_photo (GtkWidget *button, EContact *contact);

void contacts_free_list_hash (gpointer data);

GList *contacts_entries_get_values (GtkWidget *widget, GList *list);

void contacts_chooser_add_cb (GtkWidget *button);

gboolean contacts_chooser (GladeXML *xml, const gchar *title,
			   const gchar *label_markup, GList *choices,
			   GList *chosen, gboolean allow_custom,
			   GList **results);
