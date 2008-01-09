/*
 * Copyright (C) 2007 OpenedHand Ltd
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

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#define PK_NAMESPACE "org.openmoko.PhoneKit"
#define DIALER_NAMESPACE "org.openmoko.PhoneKit.Dialer"
#define DIALER_OBJECT "/org/openmoko/PhoneKit/Dialer"

#define SMS_NAMESPACE "org.openmoko.OpenmokoMessages2"
#define SMS_OBJECT "/org/openmoko/OpenmokoMessages2"

void 
openmoko_contacts_util_dial_number (const gchar *number)
{
  DBusGConnection *conn;
  DBusGProxy *proxy;
  GError *err = NULL;

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, &err);
  if (conn == NULL)
  {
    g_warning ("Failed to make DBus connection: %s", err->message);
    g_error_free (err);
    return;
  }

  proxy = dbus_g_proxy_new_for_name (conn,
                                     PK_NAMESPACE,
                                     DIALER_OBJECT,
                                     DIALER_NAMESPACE);
  if (proxy == NULL)
  {
    g_warning ("Unable to get dialer object");
    return;
  }

  err = NULL;
  dbus_g_proxy_call (proxy, "Dial", &err,
                     G_TYPE_STRING, number,
                     G_TYPE_INVALID, G_TYPE_INVALID);

  if (err)
  {
    g_warning (err->message);
    g_error_free (err);
  }
}

void
openmoko_contacts_util_sms (const gchar *uid)
{
  DBusGConnection *conn;
  DBusGProxy *proxy;
  GError *err = NULL;

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, &err);
  if (conn == NULL)
  {
    g_warning ("Failed to make DBus connection: %s", err->message);
    g_error_free (err);
    return;
  }

  proxy = dbus_g_proxy_new_for_name (conn,
                                     SMS_NAMESPACE,
                                     SMS_OBJECT,
                                     SMS_NAMESPACE);
  if (proxy == NULL)
  {
    g_warning ("Unable to get openmoko-messages2 object");
    return;
  }

  err = NULL;
  dbus_g_proxy_call (proxy, "SendMessage", &err,
                     G_TYPE_STRING, uid, G_TYPE_STRING, NULL,
                     G_TYPE_STRING, NULL,
                     G_TYPE_INVALID, G_TYPE_INVALID);

  if (err)
  {
    g_warning (err->message);
    g_error_free (err);
  }
}

