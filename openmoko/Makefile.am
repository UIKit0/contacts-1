
INCLUDES = -I$(top_srcdir)/libhito

noinst_PROGRAMS=openmoko-contacts


AM_CFLAGS = -Wall -export-dynamic $(CONTACTS_CFLAGS)
AM_CPPFLAGS=-DPKGDATADIR=\"$(pkgdatadir)\"				\
	-DCONTACTS_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"  	\
	-DDATADIR=\""$(datadir)"\"

openmoko_contacts_SOURCES=openmoko-contacts.c
openmoko_contacts_LDADD=$(top_srcdir)/libhito/libhito.a $(CONTACTS_LIBS)
openmoko_contacts_LDFLAGS = @CONTACTS_LIBS@

