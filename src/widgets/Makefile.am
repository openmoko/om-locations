themedir = $(pkgdatadir)

AM_CPPFLAGS =			\
	$(EFL_CFLAGS)		\
	$(EDBUS_CFLAGS)		\
	-I$(top_srcdir)/dbus	\
	-DTHEMEDIR=\"$(themedir)\"

noinst_LTLIBRARIES = libwidget.la

noinst_HEADERS =		\
	e_nav_button_bar.h	\
	e_nav_dialog.h		\
	e_nav_contact_editor.h	\
	e_nav_entry.h		\
	e_nav_list.h		\
	e_nav_tree_model.h	\
	debug.h

libwidget_la_SOURCES =		\
	e_nav_button_bar.c	\
	e_nav_dialog.c		\
	e_nav_contact_editor.c	\
	e_nav_entry.c		\
	e_nav_list.c		\
	e_nav_tree_model.c

libwidget_la_LIBADD = $(EFL_LIBS)
