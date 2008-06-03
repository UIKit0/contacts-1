INCLUDES = 						\
	-I$(top_srcdir)

AM_CPPFLAGS=-DPKGDATADIR=\"$(pkgdatadir)\"				\
	-DCONTACTS_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"  	\
	-DDATADIR=\""$(datadir)"\"

AM_CFLAGS = -Wall -export-dynamic $(CONTACTS_CFLAGS) $(DBUS_CFLAGS)

if HAVE_DBUS
dbus_binding_sources = contacts-dbus.xml

contacts-dbus-glue.h: $(dbus_binding_sources)
	$(LIBTOOL) --mode=execute $(DBUSBINDINGTOOL) --prefix=contacts_dbus \
	--mode=glib-server --output=contacts-dbus-glue.h $(srcdir)/$(dbus_binding_sources)

dbus_sources = contacts-dbus.c contacts-dbus.h
dbus_generated = contacts-dbus-glue.h
endif

bin_PROGRAMS = contacts

contacts_SOURCES = \
		    $(dbus_sources) \
                    bacon-message-connection.c \
                    bacon-message-connection.h \
                    contacts-callbacks-ebook.c \
                    contacts-callbacks-ebook.h \
                    contacts-callbacks-ui.c \
                    contacts-callbacks-ui.h \
                    contacts-defs.h \
                    contacts-edit-pane.c \
                    contacts-edit-pane.h \
                    contacts-main.c \
                    contacts-main.h \
                    contacts-utils.c \
                    contacts-utils.h \
                    contacts-ui.c \
                    contacts-ui.h \
                    contacts-@FRONTEND@.c \
                    contacts-@FRONTEND@.h

contacts_LDADD   = $(CONTACTS_LIBS) $(DBUS_LIBS)

contacts_LDFLAGS = @CONTACTS_LIBS@ @DBUS_LIBS@

BUILT_SOURCES = \
	$(dbus_generated)

MAINTAINERCLEANFILES = config.h.in Makefile.in

CLEANFILES = \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	$(dbus_binding_sources) \
	$(dbus_sources)

