-include $(SFS_LIB_MK)

noinst_PROGRAMS = mongotest
$(PROGRAMS): $(LDEPS_PUB) $(LDEPS_RFN)

tamed_in = mongotest.T
tamed_out = mongotest.C
mongotest_SOURCES = fakeblob_prot.C mongotest.T
mongotest_LDADD = $(LDADD_AMONGO)

SUFFIXES = .T .C .h

RPCCIN = fakeblob.x
RPCCOUT = fakeblob.C fakeblob.h

.T.C:
	-$(TAME) -o $@ $< || rm -f $@

.x.h:
	$(XMLRPCC) -h -o $@ $< || (rm -f $@ && false)
.x.C:
	$(XMLRPCC) -c -o $@ $< || (rm -f $@ && false)

fakeblob_prot.h: fakeblob_prot.x
fakeblob_prot.C: fakeblob_prot.x
fakeblob_prot.o: fakeblob_prot.h
fakeblob_prot.lo: fakeblob_prot.h

MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = core *.core *~ $(tamed_out)
EXTRA_DIST = $(tamed_in)

dist-hook:
	cd $(distdir) && rm -f $(tamed_out)

.PHONY: tameclean

tameclean:
	rm -f $(tamed_out)
