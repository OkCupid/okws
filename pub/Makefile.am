
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS_PUB)

noinst_HEADERS = pubd.h

okwsbin_PROGRAMS = pub3stage1
okwsexec_PROGRAMS = pubd
pub3stage1_SOURCES = pub3stage1.C
pubd_SOURCES = pubd.C pubserv.C cache.C
SUFFIXES = .T .C .h
pub3stage1_LDADD = $(LDADD_PUB)
pubd_LDADD = $(LDADD_PUB)

tamed_in = pubd.T pubserv.T cache.T pub3stage1.T
tamed_out = pubd.C pubserv.C cache.C pub3stage1.C

.T.C:
	-$(TAME) -o $@ $< || rm -f $@

CLEANFILES = core *.core *~ $(tamed_out)
EXTRA_DIST = .cvsignore $(tamed_in)
MAINTAINERCLEANFILES = Makefile.in

dist-hook:
	cd $(distdir) && rm -f $(tamed_out)

.PHONY: tameclean 

tameclean:
	rm -f $(tamed_out)
