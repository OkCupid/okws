
$(PROGRAMS): $(LDEPS)

noinst_HEADERS = okld.h okd.h okmgr.h 

okwsbin_PROGRAMS = okld okmgr
okwsexec_PROGRAMS =  okd
okd_SOURCES = okd.C mgrsrv.C child.C shutdown.C 
okmgr_SOURCES = okmgr.C
okld_SOURCES = okld.C okldch.C okld_script.C

SUFFIXES = .C .T .h

.T.C:
	-$(TAME) -o $@ $< || rm -f $@

okld.o:    okld.C
okld.lo:   okld.C
okldch.o:  okldch.C
okldch.lo: okldch.C
okd.o:     okd.C
okd.lo:    okd.C
okmgr.o:   okmgr.C
okmgr.lo:  okmgr.C

TAMEOUT = okld.C okldch.C okd.C okmgr.C

CLEANFILES = core *.core *~ $(TAMEOUT)
EXTRA_DIST = .cvsignore okld.T okldch.T okd.T okmgr.T
MAINTAINERCLEANFILES = Makefile.in

.PHONY: tameclean

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)
