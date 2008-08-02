ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess config.h.in \
		       config.sub configure depcomp install-sh ltmain.sh \
		       missing config.rpath mkinstalldirs 

SUBDIRS = dbus src po data tools

filesdir = $(pkgdatadir)

applicationsdir = $(datadir)/applications
applications_DATA = om-locations.desktop


EXTRA_DIST =				\
	$(applications_DATA)

clean-local:
	rm -rf *.edj *~
