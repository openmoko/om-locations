ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess config.h.in \
		       config.sub configure depcomp install-sh ltmain.sh \
		       missing config.rpath mkinstalldirs 

SUBDIRS = dbus src po images data

EDJE_CC = @edje_cc
EDJE_FLAGS = -v \
-id $(top_srcdir)/images \
-fd $(top_srcdir)/fonts

filesdir = $(pkgdatadir)
files_DATA = diversity_nav.edj

applicationsdir = $(datadir)/applications
applications_DATA = diversity-nav.desktop

EXTRA_DIST =				\
	$(files_DATA)			\
	$(applications_DATA)		\
	diversity_nav.edc		\
	fonts/Vera.ttf			\
	fonts/VeraBd.ttf

%.edj: %.edc
	$(EDJE_CC) $(EDJE_FLAGS) $< $@
	
clean-local:
	rm -rf *.edj *~
