/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * contacts
 * Copyright (C) Gilles Dartiguelongue 2008 <gdartiguelongue@comwax.com>
 *
 * contacts is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * contacts is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "contacts-dbus.h"
#include "contacts-defs.h"
#include "contacts-callbacks-ui.h"
#include "contacts-utils.h"

/* Non-generated functions */
static void contacts_dbus_new_contact  (void);

/* Set up the DBus GObject to use */
typedef struct _ContactsDbusClass ContactsDbusClass;
typedef struct _ContactsDbus ContactsDbus;

typedef struct _ContactsDbusPrivate ContactsDbusPrivate;

/*struct _ContactsDbusPrivate {

	guint session_reconnect_timeout_id;

	guint is_session_service_owner : 1;
	guint register_name : 1;
};*/

struct _ContactsDbus {
	GObject parent;

	ContactsData *data;
};

struct _ContactsDbusClass {
	GObjectClass parent;
};

#define CONTACTS_DBUS_SERVICE           "org.gnome.Contacts"
#define CONTACTS_DBUS_PATH              "/org/gnome/Contacts"
#define CONTACTS_DBUS_INTERFACE         "org.gnome.Contacts"

#define CONTACTS_TYPE_DBUS             (contacts_dbus_get_type ())
#define CONTACTS_DBUS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONTACTS_TYPE_DBUS, ContactsDbus))
#define CONTACTS_DBUS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CONTACTS_TYPE_DBUS, ContactsDbusClass))
#define CONTACTS_IS_DBUS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CONTACTS_TYPE_DBUS))
#define CONTACTS_IS_DBUS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CONTACTS_TYPE_DBUS))
#define CONTACTS_DBUS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CONTACTS_TYPE_DBUS, ContactsDbusClass))

#define CONTACTS_DBUS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CONTACTS_TYPE_DBUS, ContactsDbusPrivate))

G_DEFINE_TYPE (ContactsDbus, contacts_dbus, G_TYPE_OBJECT);

static void contacts_dbus_view_contact (ContactsDbus *obj,const gchar *econtact_uid);
static void contacts_dbus_edit_contact (ContactsDbus *obj,const gchar *econtact_uid);

#include "contacts-dbus-glue.h"

static ContactsDbus	*contacts_dbus = NULL;

//static DBusGConnection *session_bus;

static void
contacts_dbus_init (ContactsDbus *object)
{
}

static void
contacts_dbus_class_init (ContactsDbusClass *klass)
{
}

/*
 * Freedesktop
 */

#define FREEDESKTOP_DBUS_SERVICE      "org.freedesktop.DBus"
#define FREEDESKTOP_DBUS_PATH         "/org/freedesktop/DBus"
#define FREEDESKTOP_DBUS_INTERFACE    "org.freedesktop.DBus"

/* Static method */

gboolean
contacts_dbus_init_for_session (ContactsData *data)
{
	DBusGConnection *bus;
	DBusGProxy      *proxy;
	GError          *error = NULL;
	guint            result;

	g_message ("Initialising Contacts object");

	bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (!bus) {
		g_warning ("Couldn't connect to the session bus");
		return FALSE;
	}

	proxy = dbus_g_proxy_new_for_name (bus,
						   FREEDESKTOP_DBUS_SERVICE,
						   FREEDESKTOP_DBUS_PATH,
						   FREEDESKTOP_DBUS_INTERFACE);
	if (!proxy) {
		return FALSE;
	}

	dbus_g_object_type_install_info (CONTACTS_TYPE_DBUS, &dbus_glib_contacts_dbus_object_info);

	contacts_dbus = g_object_new (CONTACTS_TYPE_DBUS, NULL);
	contacts_dbus->data = data;

	dbus_g_connection_register_g_object (bus, CONTACTS_DBUS_PATH, G_OBJECT (contacts_dbus));

	if (!org_freedesktop_DBus_request_name (proxy,
						CONTACTS_DBUS_SERVICE,
						DBUS_NAME_FLAG_DO_NOT_QUEUE,
						&result, &error)) {
		g_warning ("Failed to acquire %s: %s",
			   CONTACTS_DBUS_SERVICE,
			   error->message);
		g_error_free (error);
		return FALSE;
	}

	g_message ("Ready");

	return TRUE;
}


/* Exported DBus functions */

static void
contacts_dbus_new_contact  (void)
{
	contacts_new_cb (NULL, contacts_dbus->data);
	contacts_window_present (contacts_dbus->data);
}

static void
contacts_dbus_view_contact (ContactsDbus *obj, const gchar *econtact_uid)
{
	g_message ("View contact");
	contacts_set_selected_contact (contacts_dbus->data, econtact_uid);
	contacts_search_changed_cb (NULL, contacts_dbus->data);
	contacts_window_present (contacts_dbus->data);
}

static void
contacts_dbus_edit_contact (ContactsDbus *obj, const gchar *econtact_uid)
{
	g_message ("Edit contact %s", econtact_uid);
	contacts_set_selected_contact (contacts_dbus->data, econtact_uid);
	contacts_search_changed_cb (NULL, contacts_dbus->data);
	contacts_edit_cb (NULL, contacts_dbus->data);
	contacts_window_present (contacts_dbus->data);
}
