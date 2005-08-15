AM_CPPFLAGS=-DPKGDATADIR=\"$(pkgdatadir)\"
AM_CFLAGS = -Wall -g  $(CONTACTS_CFLAGS)

bin_PROGRAMS = contacts

contacts_SOURCES = main.c utils.c callbacks-ui.c callbacks-ebook.c \
                   utils.h callbacks-ui.h callbacks-ebook.h globals.h main.h
contacts_LDADD   = $(CONTACTS_LIBS)

MAINTAINERCLEANFILES = config.h.in Makefile.in
