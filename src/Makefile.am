SUBDIRS = widgets

localedir = $(datadir)/locale
themedir = $(pkgdatadir)
mapsdir = $(pkgdatadir)/maps

AM_CPPFLAGS =				\
	$(EFL_CFLAGS)			\
	$(EDBUS_CFLAGS)			\
	-I$(top_srcdir)/dbus		\
	-DLOCALEDIR=\"$(localedir)\"	\
	-DTHEMEDIR=\"$(themedir)\"	\
	-DMAPSDIR=\"$(mapsdir)\"

noinst_HEADERS =		\
	e_mod_main.h		\
	e_mod_config.h		\
	e_mod_nav.h		\
	e_nav_theme.h		\
	e_nav.h			\
	e_spiralmenu.h		\
	e_flyingmenu.h		\
	e_nav_tileset.h		\
	e_nav_item_ap.h		\
	e_nav_item_neo_me.h	\
	e_nav_item_neo_other.h	\
	e_nav_item_link.h	\
	e_nav_item_location.h	\
	e_nav_dbus.h		\
	e_ctrl.h		\
	tileman.h		\
	e_nav_misc.h

bin_PROGRAMS = om-locations
om_locations_SOURCES =		\
	e_mod_main.c		\
	e_mod_config.c		\
	e_mod_nav.c		\
	e_nav_theme.c		\
	e_nav.c			\
	e_spiralmenu.c		\
	e_flyingmenu.c		\
	e_nav_tileset.c		\
	e_nav_item_ap.c		\
	e_nav_item_neo_me.c	\
	e_nav_item_neo_other.c	\
	e_nav_item_link.c	\
	e_nav_item_location.c	\
	e_nav_dbus.c		\
	e_ctrl.c		\
	tileman.c		\
	e_nav_misc.c

om_locations_LDADD =				\
	$(EFL_LIBS)				\
	$(EDBUS_LIBS)				\
	$(top_builddir)/dbus/libedbus-proxy.la	\
	widgets/libwidget.la
