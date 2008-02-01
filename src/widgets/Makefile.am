MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS =			\
	$(EFL_CFLAGS)		\
	-DTHEME_PATH=\"$(pkgdatadir)\"

noinst_LTLIBRARIES = libwidget.la

noinst_HEADERS =		\
        e_ilist.h               \
        e_box.h                 \
        e_icon.h                \
        e_ctrl_dialog.h         \
        e_nav_theme.h           \
        debug.h                 


libwidget_la_SOURCES =		\
        e_ilist.c               \
	e_box.c                 \
        e_icon.c                \
        e_ctrl_dialog.c         \
        e_nav_theme.c           

libwidget_la_LIBADD = $(EFL_LIBS) 

