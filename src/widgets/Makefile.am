MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS =			\
	$(EFL_CFLAGS)		\
	$(EDBUS_CFLAGS)		\
	-I$(top_srcdir)/dbus	\
	-DTHEME_PATH=\"$(pkgdatadir)\"

noinst_LTLIBRARIES = libwidget.la

noinst_HEADERS =		\
        e_nav_titlepane.h       \
        e_nav_theme.h           \
        e_widget_textblock.h    \
        e_widget.h              \
        e_nav_dialog.h          \
        e_nav_alert.h           \
        e_nav_textedit.h        \
        e_entry.h               \
        e_editable.h            \
        debug.h                 


libwidget_la_SOURCES =		\
        e_nav_titlepane.c       \
        e_nav_theme.c           \
        e_widget_textblock.c    \
        e_nav_dialog.c          \
        e_nav_alert.c           \
        e_nav_textedit.c        \
        e_entry.c               \
        e_editable.c            \
        e_widget.c              

libwidget_la_LIBADD = $(EFL_LIBS) 

