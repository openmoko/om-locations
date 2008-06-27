ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = widgets

mapdir = $(pkgdatadir)/maps
diversity_mapdir = $(datadir)/diversity-nav/maps

AM_CPPFLAGS =					\
	$(EFL_CFLAGS)				\
	$(EDBUS_CFLAGS)				\
	-I$(top_srcdir)/dbus			\
	-DTHEME_PATH=\"$(pkgdatadir)\"		\
        -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"	\
	-DMAP_PATH=\"$(mapdir)\"                \
	-DDIVERSITY_MAP_PATH=\"$(diversity_mapdir)\"

noinst_HEADERS =		\
	e_mod_main.h		\
        e_mod_config.h          \
	e_mod_nav.h		\
	e_nav_theme.h		\
	e_nav.h			\
	e_spiralmenu.h		\
	e_flyingmenu.h		\
        e_spreadmenu.h          \
	e_nav_tileset.h		\
        e_nav_taglist.h         \
	e_nav_item_ap.h		\
	e_nav_item_neo_me.h	\
	e_nav_item_neo_other.h	\
	e_nav_item_link.h	\
        e_nav_item_location.h   \
	e_nav_dbus.h            \
        e_ctrl.h                \
        e_nav_misc.h

bin_PROGRAMS = splinter
splinter_SOURCES =		\
	e_mod_main.c		\
        e_mod_config.c          \
	e_mod_nav.c		\
	e_nav_theme.c		\
	e_nav.c			\
	e_spiralmenu.c		\
	e_flyingmenu.c		\
	e_spreadmenu.c		\
	e_nav_tileset.c		\
        e_nav_taglist.c         \
	e_nav_item_ap.c		\
	e_nav_item_neo_me.c	\
	e_nav_item_neo_other.c	\
	e_nav_item_link.c	\
        e_nav_item_location.c   \
	e_nav_dbus.c            \
        e_ctrl.c                \
        e_nav_misc.c
		    
splinter_LDADD =		  \
	$(EFL_LIBS)		  \
	$(EDBUS_LIBS)		  \
	../dbus/libedbus-proxy.la \
        widgets/libwidget.la

clean-local:
	rm -rf *~
