SUBDIRS = fonts images

EDJE_CC = @edje_cc@
EDJE_FLAGS =			\
	-v			\
	-id $(srcdir)/images	\
	-fd $(srcdir)/fonts

themedir = $(pkgdatadir)
theme_DATA = default.edj

EXTRA_DIST =			\
	diversity_nav.edc	\
	diversity_neo_me.edc	\
	default_nav_dialog.edc	\
	default_nav_alert.edc	\
	default_nav_button.edc	\
	default_textblock.edc	\
	default.edc

default.edj: $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS)	\
	$(srcdir)/default.edc		\
	default.edj

clean-local:
	rm -f default.edj
