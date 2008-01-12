ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess config.h.in \
		       config.sub configure depcomp install-sh ltmain.sh \
		       missing config.rpath mkinstalldirs 

SUBDIRS = src po maps

EDJE_CC = @edje_cc
EDJE_FLAGS = -v \
-id $(top_srcdir)/images \
-fd $(top_srcdir)/fonts

filesdir = $(datadir)
files_DATA = module.desktop \
	     e-module-diversity_nav.edj \
	     diversity_nav.edj
	     
EXTRA_DIST = $(files_DATA) \
	     diversity_nav.edc \
	     e-module-diversity_nav.edc \
	     fonts/Vera.ttf \
	     fonts/VeraBd.ttf \
	     images/module_icon.png \
	     images/logo.png \
	     images/measure_64px.png \
	     images/city.png \
	     images/zoom.png

%.edj: %.edc
	$(EDJE_CC) $(EDJE_FLAGS) $< $@
	
clean-local:
	rm -rf *.edj *~
	
uninstall-hook:
	rm -rf $(datadir)
