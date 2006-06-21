
$(PROGRAMS): $(LDEPS)

noinst_PROGRAMS = hcli hsrv degz mygz pt1cli okwc tmr bench mygz2 enctst okwc2

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

SUFFIXES = .C .T .h

.T.C:
	-$(TAME) -c $< || rm -f $@

okwcmain2.o:	okwcmain2.C
okwcmain2.lo:	okwcmain2.C

okwc2_SOURCES = okwcmain2.C

mygz2_SOURCES = mygz2.C
#mygz2_LDADD = $(LIBASYNC)

BUILT_SOURCES = okwcmain2.C
CLEANFILES = core *.core *~
EXTRA_DIST = .cvsignore okwcmain2.T
MAINTAINERCLEANFILES = Makefile.in
