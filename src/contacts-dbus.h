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

#ifndef _CONTACTS_DBUS_H_
#define _CONTACTS_DBUS_H_

#include <dbus/dbus-glib.h>
#include "contacts-defs.h"

G_BEGIN_DECLS

gboolean	contacts_dbus_init_for_session		(ContactsData *data);

void		contacts_dbus_finalize_for_session	(void);

G_END_DECLS

#endif /* _CONTACTS_DBUS_H_ */
