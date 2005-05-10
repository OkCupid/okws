
$(PROGRAMS): $(LDEPS)

noinst_HEADERS = okld.h okd.h okmgr.h

okwsbin_PROGRAMS = okld okmgr
okwsexec_PROGRAMS =  okd
okd_SOURCES = okd.C mgrsrv.C child.C shutdown.C
okmgr_SOURCES = okmgr.C
okld_SOURCES = okld.C okldch.C

CLEANFILES = core *.core *~
EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in
