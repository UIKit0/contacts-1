uidir = $(pkgdatadir)
ui_DATA = contacts.glade

applicationdir = $(datadir)/applications
application_DATA = contacts.desktop

icondir = $(datadir)/pixmaps/
icon_DATA = oh-contacts.png

schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = contacts.schemas

install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
		$(GCONFTOOL) --makefile-install-rule $(schema_DATA) || \
		(echo ;\
		echo "*****************************************************"; \
		echo "Installation of schemas failed, install them manually"; \
		echo "*****************************************************";)
		@true

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = $(ui_DATA) $(application_DATA) $(icon_DATA) $(schema_DATA)
