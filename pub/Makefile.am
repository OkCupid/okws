
$(PROGRAMS): $(LDEPS_PUB)

bin_PROGRAMS = pub 
okwsexec_PROGRAMS = pubd
pub_SOURCES = pubmain.C
pub_LDADD = $(LDADD_PUB)
pubd_SOURCES = pubd.C pubserv.C
pubd_LDADD = $(LDADD_PUB)

CLEANFILES = core *.core *~
EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in
