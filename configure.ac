AC_PREREQ(2.53)
AC_INIT(contacts, 0.1, http://www.openedhand.com/)
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(src/contacts-main.c)
AM_CONFIG_HEADER(src/config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(CONTACTS, glib-2.0 glib-2.0 gtk+-2.0 libglade-2.0 libebook-1.2)

old_cflags=$CFLAGS
CFLAGS=$CONTACTS_CFLAGS
AC_CHECK_TYPE(EContactPhotoType, [], [], [#include <libebook/e-book.h>])
CFLAGS=$old_cflags

if test $ac_cv_type_EContactPhotoType = yes; then
	AC_DEFINE(HAVE_PHOTO_TYPE, 1, [Defined if EContactPhotoType exists])
fi

AC_ARG_ENABLE(gnome-vfs, [  --enable-gnome-vfs      Use gnome-vfs for importing and exporting vCards [[default=no]]],
 [want_gnomevfs="$enableval"])

if test "$want_gnomevfs" = "yes"; then
	PKG_CHECK_MODULES(GNOMEVFS, gnome-vfs-2.0)
	AC_DEFINE(HAVE_GNOMEVFS, 1, [Define if you want Gnome VFS support])
	CONTACTS_CFLAGS="$CONTACTS_CFLAGS $GNOMEVFS_CFLAGS"
	CONTACTS_LIBS="$CONTACTS_LIBS $GNOMEVFS_LIBS"
fi

AC_OUTPUT([
Makefile
data/Makefile
src/Makefile
])
