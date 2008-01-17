ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I. \
	 -I$(top_srcdir) \
	 -I$(includedir) \
	 @e_cflags@

AM_CPPFLAGS = $(EDBUS_FLAGS) -I$(top_srcdir)/dbus -DHAVE_EDBUS

pkgdir = $(datadir)/$(MODULE_ARCH)
pkg_LTLIBRARIES = module.la
module_la_SOURCES = e_mod_main.h \
		    e_mod_main.c \
		    e_mod_nav.h \
		    e_mod_nav.c \
		    e_nav.h \
		    e_nav.c \
		    e_spiralmenu.h \
		    e_spiralmenu.c \
		    e_nav_item_ap.h \
		    e_nav_item_ap.c \
		    e_nav_item_neo_me.h \
		    e_nav_item_neo_me.c \
		    e_nav_item_neo_other.h \
		    e_nav_item_neo_other.c \
		    e_nav_item_link.h \
		    e_nav_item_link.c \
                    e_nav_dbus.c \
                    e_nav_dbus.h
		    
module_la_LIBADD = @e_libs@ $(EDBUS_LIBS) ../dbus/libedbus-proxy.la
module_la_LDFLAGS = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

clean-local:
	rm -rf *~
