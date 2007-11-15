SUBDIRS = data

INCLUDES = -I$(top_srcdir)/libhito

bin_PROGRAMS=openmoko-contacts


AM_CFLAGS = -Wall -export-dynamic $(CONTACTS_CFLAGS) $(MOKOUI_CFLAGS)
AM_CPPFLAGS=-DPKGDATADIR=\"$(pkgdatadir)\"				\
	-DCONTACTS_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"  	\
	-DDATADIR=\""$(datadir)"\"

openmoko_contacts_SOURCES= \
	contacts-history.c \
	contacts-history.h \
	openmoko-contacts.c \
	openmoko-contacts.h \
	openmoko-contacts-details.c \
	openmoko-contacts-details.h \
	openmoko-contacts-groups.c \
	openmoko-contacts-groups.h \
	openmoko-contacts-history.c \
	openmoko-contacts-history.h \
	openmoko-contacts-list.c \
	openmoko-contacts-list.h \
	openmoko-contacts-util.c \
	openmoko-contacts-util.h \
	koto-cell-renderer-pixbuf.c \
	koto-cell-renderer-pixbuf.h \
	gbase64.c \
	gbase64.h \
	contacts-attribute-store.h \
	contacts-attribute-store.c




openmoko_contacts_LDADD=$(top_srcdir)/libhito/libhito.a $(CONTACTS_LIBS) \
			$(MOKOUI_LIBS)
openmoko_contacts_LDFLAGS = @CONTACTS_LIBS@ @MOKOUI_LIBS@

