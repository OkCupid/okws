
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS_PUB) $(LDEPS_RFN)

noinst_HEADERS = pubd.h

okwsbin_PROGRAMS = pub3stage1 pub3trace_replayer pub3astdumper pub3
okwsexec_PROGRAMS = pubd
pub3trace_replayer_SOURCES = pub3trace_replayer.C
pub3_SOURCES = pub3.C
pub3stage1_SOURCES = pub3stage1.C
pubd_SOURCES = pubd.C pubserv.C cache.C
pub3astdumper_SOURCES = pub3astdumper.C
SUFFIXES = .T .C .h
pub3trace_replayer_LDADD = $(LDADD_RFN)
pub3_LDADD = $(LDADD_RFN)
pub3stage1_LDADD = $(LDADD_PUB)
pubd_LDADD = $(LDADD_PUB)
pub3astdumper_LDADD = $(LDADD_PUB)

tamed_in = pubd.T pubserv.T cache.T pub3stage1.T pub3trace_replayer.T pub3.T
tamed_out = pubd.C pubserv.C cache.C pub3stage1.C pub3trace_replayer.C pub3.C

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
