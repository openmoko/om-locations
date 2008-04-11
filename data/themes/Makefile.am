MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = fonts images

EDJE_CC = @edje_cc@
EDJE_FLAGS = \
-v \
-id $(top_srcdir)/data/themes/images \
-fd $(top_srcdir)/data/themes/fonts 

filesdir = $(pkgdatadir)
files_DATA = \
default.edj

EXTRA_DIST = \
diversity_nav.edc \
default_nav_dialog.edc \
default_nav_alert.edc \
default_nav_button.edc \
default_titlepane.edc \
default_textblock.edc \
default_entry.edc \
default.edc

default.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default.edc \
	$(top_builddir)/data/themes/default.edj

clean-local:
	rm -f *.edj
