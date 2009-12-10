
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

noinst_HEADERS = okld.h okd.h okmgr.h 

okwsbin_PROGRAMS = okld okmgr
okwsexec_PROGRAMS =  okd
okd_SOURCES = okd.C child.C shutdown.C stats.C
okmgr_SOURCES = okmgr.C
okld_SOURCES = okld.C okldch.C okld_script.C

SUFFIXES = .C .T .h

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

okld.o:    okld.C
okld.lo:   okld.C
okldch.o:  okldch.C
okldch.lo: okldch.C
okd.o:     okd.C
okd.lo:    okd.C
okmgr.o:   okmgr.C
okmgr.lo:  okmgr.C
child.o:   child.C
child.lo:  child.C

TAMEOUT = okld.C okldch.C okd.C okmgr.C child.C stats.C

CLEANFILES = core *.core *~ $(TAMEOUT)
EXTRA_DIST = .cvsignore okld.T okldch.T okd.T okmgr.T child.T stats.T
MAINTAINERCLEANFILES = Makefile.in

.PHONY: tameclean

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)
