SUBDIRS = src po data

EXTRA_DIST = intltool-extract.in intltool-merge.in intltool-update.in

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing

snapshot:
        $(MAKE) dist distdir=$(PACKAGE)-snap`date +"%Y%m%d"`

