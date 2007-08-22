INCLUDES = -I$(top_srcdir)/libhito

bin_PROGRAMS=hildon-contacts

AM_CFLAGS = -Wall -export-dynamic $(CONTACTS_CFLAGS) $(HILDON_CFLAGS)
AM_CPPFLAGS=-DPKGDATADIR=\"$(pkgdatadir)\"				\
	-DCONTACTS_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"  	\
	-DDATADIR=\""$(datadir)"\"

hildon_contacts_LDADD=$(top_srcdir)/libhito/libhito.a $(CONTACTS_LIBS) \
			$(HILDON_LIBS)
hildon_contacts_LDFLAGS = @CONTACTS_LIBS@ @HILDON_LIBS@
