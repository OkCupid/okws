
$(PROGRAMS): $(LDEPS)

noinst_PROGRAMS = hcli hsrv degz mygz pt1cli okwc tmr bench mygz2 enctst

noinst_HEADERS = normdist.h

okwsbin_PROGRAMS = okdbg

okdbg_SOURCES = okdbg.C
okdbg_LDADD = $(LIBASYNC) $(LIBPY)

hcli_SOURCES = hcli.C

okwc_SOURCES = okwcmain.C

pt1cli_SOURCES = pt1cli.C normdist.C
#pt1cli_LDFLAGS = -all-static

hsrv_SOURCES = hsrv.C

degz_SOURCES = degz.C 

mygz_SOURCES = mygz.C

tmr_SOURCES = tmr.C

bench_SOURCES = bench.C

enctst_SOURCES = enctst.C

mygz2_SOURCES = mygz2.C
#mygz2_LDADD = $(LIBASYNC)

CLEANFILES = core *.core *~
EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in
