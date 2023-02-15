## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign
SUBDIRS = man src

EXTRA_DIST = LICENSE INSTALL README ChangeLog

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure configuration.h.in \
                       stamp-h.in

dist-sign:
	gpg -ba $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz

.PHONY: dist-all
dist-all: dist dist-sign

dist-all-clean:
	rm -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz
	rm -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz.asc
