ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS =			\
	$(EFL_CFLAGS)

noinst_PROGRAMS = diversity-map-pack
diversity_map_pack_SOURCES =	\
	diversity-map-pack.c
		    
diversity_map_pack_LDADD =	\
	$(EFL_LIBS)

clean-local:
	rm -rf *~
