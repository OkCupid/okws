
$(PROGRAMS): $(LDEPS_PUB)

noinst_HEADERS = pubd.h pubd2.h

okwsbin_PROGRAMS = pub treestat
okwsexec_PROGRAMS = pubd
pub_SOURCES = pubmain.C
pub_LDADD = $(LDADD_PUB)
pubd_SOURCES = pubd.C pubserv.C pubserv2.C cache2.C 
pubd_LDADD = $(LDADD_PUB)
treestat_SOURCES = treestat.C
treestat_LDADD = $(LDADD_PUB)

SUFFIXES = .T .C .h

.T.C:
	-$(TAME) -c $< || rm -f $@

cache2.o:	cache2.C
cache2.lo:	cache2.C
pubd.o:		pubd.C
pubd.lo:	pubd.C
pubserv2.o:	pubserv2.C
pubserv2.lo:	pubserv2.C
treestat.o:	treestat.C
treestat.lo:	treestat.C

CLEANFILES = core *.core *~
EXTRA_DIST = .cvsignore cache2.T pubserv2.T pubd.T treestat.T
BUILT_SOURCES = cache2.C pubserv2.C pubd.C treestat.C
MAINTAINERCLEANFILES = Makefile.in

.PHONY: tameclean 

tameclean:
	rm -f $(srcdir)/cache2.C $(srcdir)/pubd.C $(srcdir)/treestat.C \
		$(srcdir)/pubserv2.C
