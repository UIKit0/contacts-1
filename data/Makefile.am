desktopdir = $(datadir)/applications
desktop_in_files = contacts.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_in_files)

icondir = $(datadir)/icons/hicolor/48x48/apps
dist_icon_DATA = contacts.png

svnicondir = $(datadir)/icons/hicolor/scalable/apps
dist_svnicon_DATA = contacts.svg

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

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

install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
			echo "Updating Gtk icon cache."; \
			$(gtk_update_icon_cache); \
		else \
			echo "*** Icon cache not updated.	After install, run this:"; \
			echo "***	 $(gtk_update_icon_cache)"; \
		fi

CLEANFILES = $(desktop_DATA)

MAINTAINERCLEANFILES = Makefile.in
