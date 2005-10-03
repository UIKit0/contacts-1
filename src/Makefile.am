AM_CPPFLAGS=-DPKGDATADIR=\"$(pkgdatadir)\"
AM_CFLAGS = -Wall -export-dynamic $(CONTACTS_CFLAGS)

bin_PROGRAMS = contacts

contacts_SOURCES = contacts-main.c contacts-edit-pane.c               \
                   contacts-utils.c contacts-callbacks-ui.c           \
                   contacts-callbacks-ebook.c contacts-utils.h        \
                   contacts-callbacks-ui.h contacts-callbacks-ebook.h \
                   contacts-main.h contacts-edit-pane.h contacts-defs.h

contacts_LDADD   = $(CONTACTS_LIBS)

MAINTAINERCLEANFILES = config.h.in Makefile.in
