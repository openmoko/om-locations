themedir = $(pkgdatadir)

AM_CPPFLAGS =			\
	$(EFL_CFLAGS)		\
	$(EDBUS_CFLAGS)		\
	-I$(top_srcdir)/dbus	\
	-DTHEMEDIR=\"$(themedir)\"

noinst_LTLIBRARIES = libwidget.la

noinst_HEADERS =		\
	e_nav_dialog.h		\
	e_nav_alert.h		\
	e_nav_textedit.h	\
	e_nav_contact_editor.h	\
	e_nav_contact_list.h	\
	e_nav_list.h		\
	e_nav_tree_model.h	\
	debug.h

libwidget_la_SOURCES =		\
	e_nav_dialog.c		\
	e_nav_alert.c		\
	e_nav_textedit.c	\
	e_nav_contact_editor.c	\
	e_nav_contact_list.c	\
	e_nav_list.c		\
	e_nav_tree_model.c

libwidget_la_LIBADD = $(EFL_LIBS)
