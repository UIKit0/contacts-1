AC_PREREQ(2.53)
AC_INIT(contacts, 0.0, http://www.openedhand.com/)
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(src/contacts-main.c)
AM_CONFIG_HEADER(src/config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(CONTACTS, glib-2.0 glib-2.0 gtk+-2.0 libglade-2.0 libebook-1.2)

AC_OUTPUT([
Makefile
data/Makefile
src/Makefile
])
