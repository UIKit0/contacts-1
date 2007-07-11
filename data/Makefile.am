SUBDIRS = icons

desktopdir = $(datadir)/applications
desktop_in_files = contacts.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_in_files)

schemadir = $(GCONF_SCHEMA_FILE_DIR)
dist_schema_DATA = contacts.schemas

dist_man1_MANS = contacts.1

if HAVE_GCONF
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
		$(GCONFTOOL) --makefile-install-rule $(schema_DATA) || \
		(echo ;\
		echo "*****************************************************"; \
		echo "Installation of schemas failed, install them manually"; \
		echo "*****************************************************";)
		@true
endif

CLEANFILES = $(desktop_DATA)

MAINTAINERCLEANFILES = Makefile.in
