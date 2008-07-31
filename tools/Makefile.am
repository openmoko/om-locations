ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS =			\
	$(EFL_CFLAGS)

bin_PROGRAMS = splinter-map-pack
splinter_map_pack_SOURCES =	\
	diversity-map-pack.c
		    
splinter_map_pack_LDADD =	\
	$(EFL_LIBS)

clean-local:
	rm -rf *~
