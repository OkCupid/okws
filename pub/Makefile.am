
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS_PUB)

okwsbin_PROGRAMS = pub3stage1
pub3stage1_SOURCES = pub3stage1.C
SUFFIXES = .T .C .h

.T.C:
	-$(TAME) -o $@ $< || rm -f $@

CLEANFILES = core *.core *~ pub3stage1.C
EXTRA_DIST = .cvsignore pub3stage1.T
MAINTAINERCLEANFILES = Makefile.in

dist-hook:
	cd $(distdir) && rm -f pub3stage1.C

.PHONY: tameclean 

tameclean:
	rm -f pub3stage1.C
