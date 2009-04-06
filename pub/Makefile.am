
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS_PUB)

noinst_HEADERS = pubd.h pubd2.h

okwsbin_PROGRAMS = pub treestat
okwsexec_PROGRAMS = pubd
pub_SOURCES = pub3.C
pub_LDADD = $(LDADD_PUB)
pubd_SOURCES = pubd.C pubserv.C pubserv2.C cache2.C 
pubd_LDADD = $(LDADD_PUB)
treestat_SOURCES = treestat.C
treestat_LDADD = $(LDADD_PUB)

SUFFIXES = .T .C .h

.T.C:
	-$(TAME) -o $@ $< || rm -f $@

cache2.o:	cache2.C
cache2.lo:	cache2.C
pubd.o:		pubd.C
pubd.lo:	pubd.C
pubserv2.o:	pubserv2.C
pubserv2.lo:	pubserv2.C
treestat.o:	treestat.C
treestat.lo:	treestat.C
pub3.o:		pub3.C
pub3.lo:	pub3.C

CLEANFILES = core *.core *~ cache2.C pubserv2.C pubd.C treestat.C pub3.C
EXTRA_DIST = .cvsignore cache2.T pubserv2.T pubd.T treestat.T pub3.T
MAINTAINERCLEANFILES = Makefile.in

dist-hook:
	cd $(distdir) && rm -f cache2.C pubd.C treestat.C pubserv2.C pub3.C

.PHONY: tameclean 

tameclean:
	rm -f cache2.C pubd.C treestat.C pubserv2.C
