AC_PREREQ(2.53)
AC_INIT(contacts, 0.10, http://www.pimlico-project.org/)
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(src/contacts-main.c)
AM_CONFIG_HEADER(src/config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_LIBTOOL
IT_PROG_INTLTOOL([0.35.0])

dnl i18n support
GETTEXT_PACKAGE=Contacts
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES(CONTACTS, glib-2.0 glib-2.0 gtk+-2.0 libebook-1.2)

old_cflags=$CFLAGS
CFLAGS=$CONTACTS_CFLAGS
AC_CHECK_TYPE(EContactPhotoType, [], [], [#include <libebook/e-book.h>])
CFLAGS=$old_cflags

if test $ac_cv_type_EContactPhotoType = yes; then
	AC_DEFINE(HAVE_PHOTO_TYPE, 1, [Defined if EContactPhotoType exists])
fi

AC_ARG_ENABLE(gnome-vfs,
    AC_HELP_STRING([--enable-gnome-vfs], [Disable use of gnome-vfs for importing and exporting vCards]),
   [want_gnomevfs="$enableval"], [want_gnomevfs="yes"])

if test "$want_gnomevfs" = "yes"; then
	PKG_CHECK_MODULES(GNOMEVFS, gnome-vfs-2.0)
	AC_DEFINE(HAVE_GNOMEVFS, 1, [Define if you want Gnome VFS support])
	CONTACTS_CFLAGS="$CONTACTS_CFLAGS $GNOMEVFS_CFLAGS"
	CONTACTS_LIBS="$CONTACTS_LIBS $GNOMEVFS_LIBS"
fi

AC_ARG_ENABLE(gconf,
  AC_HELP_STRING([--enable-gconf],[Disable use gconf for storing settings]),
 [want_gconf="$enableval"], [want_gconf="yes"])

if test "$want_gconf" = "yes"; then
	PKG_CHECK_MODULES(GCONF, gconf-2.0)
	AC_DEFINE(HAVE_GCONF, 1, [Define if you want Gconf support])
	CONTACTS_CFLAGS="$CONTACTS_CFLAGS $GCONF_CFLAGS"
	CONTACTS_LIBS="$CONTACTS_LIBS $GCONF_LIBS"

	# Find how and where to put the GConf schemas
	AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
	if test x"$GCONFTOOL" = xno; then
		AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
	fi
fi
AM_CONDITIONAL([HAVE_GCONF], [test "$want_gconf" = "yes"])

AC_ARG_ENABLE(platform,
    AC_HELP_STRING([--enable-platform], [Enable platform specfic code [[default=none]]]),
    platform=$enable_plaftform)

if test "$enable_platform" = "maemo"; then
  INCLUDE_MAEMO_ICONS=26x26
  AC_SUBST(INCLUDE_MAEMO_ICONS)
fi

FRONTEND="gtk"
AC_SUBST(FRONTEND)
AC_DEFINE_UNQUOTED(FRONTEND_HEADER, ["contacts-gtk.h"], [Main GTK+ Frontend])

AM_GCONF_SOURCE_2

AC_ARG_ENABLE(dbus,
    AC_HELP_STRING([--enable-dbus],
    [Enable D-Bus interface [[default=yes]]]),
    [want_dbus="$enableval"],[want_dbus=yes])

if test "x$want_dbus" = "xyes"; then
	PKG_CHECK_MODULES(DBUS, dbus-1 dbus-glib-1)

	AC_DEFINE(HAVE_DBUS, 1, [Have D-Bus support])
	AC_SUBST(DBUS_CFLAGS)
	AC_SUBST(DBUS_LIBS)
fi
AM_CONDITIONAL(HAVE_DBUS, test "x$want_dbus" = "xyes")

AC_PATH_PROG(DBUSBINDINGTOOL, dbus-binding-tool)

AC_CONFIG_FILES([
Makefile
data/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/22x22/Makefile
data/icons/24x24/Makefile
data/icons/26x26/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/icons/scalable/Makefile
po/Makefile.in
src/Makefile
])

AC_OUTPUT

