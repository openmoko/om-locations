ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = widgets

AM_CPPFLAGS =			\
	$(EFL_CFLAGS)		\
	$(EDBUS_CFLAGS)		\
	-I$(top_srcdir)/dbus	\
	-DTHEME_PATH=\"$(pkgdatadir)\"

noinst_HEADERS =		\
	e_mod_main.h		\
	e_mod_nav.h		\
	e_nav.h			\
	e_spiralmenu.h		\
	e_flyingmenu.h		\
	e_nav_tileset.h		\
        e_nav_taglist.h         \
	e_nav_item_ap.h		\
	e_nav_item_neo_me.h	\
	e_nav_item_neo_other.h	\
	e_nav_item_link.h	\
        e_nav_item_location.h   \
	e_nav_dbus.h            \
        e_ctrl.h                

bin_PROGRAMS = diversity-nav
diversity_nav_SOURCES =		\
	e_mod_main.c		\
	e_mod_nav.c		\
	e_nav.c			\
	e_spiralmenu.c		\
	e_flyingmenu.c		\
	e_nav_tileset.c		\
        e_nav_taglist.c         \
	e_nav_item_ap.c		\
	e_nav_item_neo_me.c	\
	e_nav_item_neo_other.c	\
	e_nav_item_link.c	\
        e_nav_item_location.c   \
	e_nav_dbus.c            \
        e_ctrl.c                
		    
diversity_nav_LDADD =		  \
	$(EFL_LIBS)		  \
	$(EDBUS_LIBS)		  \
	../dbus/libedbus-proxy.la \
        widgets/libwidget.la

clean-local:
	rm -rf *~
