
$(PROGRAMS): $(LDEPS)

noinst_PROGRAMS = hcli hsrv degz mygz pt1cli okwc tmr bench mygz2 enctst okwc2
okwsbin_PROGRAMS = okdbg

noinst_HEADERS = normdist.h

okdbg_SOURCES = okdbg.C
hcli_SOURCES = hcli.C
okwc_SOURCES = okwcmain.C
pt1cli_SOURCES = pt1cli.C normdist.C
hsrv_SOURCES = hsrv.C
degz_SOURCES = degz.C 
mygz_SOURCES = mygz.C
tmr_SOURCES = tmr.C
bench_SOURCES = bench.C
enctst_SOURCES = enctst.C
okwc2_SOURCES = okwcmain2.C
mygz2_SOURCES = mygz2.C

okdbg_LDADD = $(LIBASYNC) $(LDADD_THR_SFS) $(LIBPY)

SUFFIXES = .C .T .h

.T.C:
	-$(TAME) -o $@ $< || rm -f $@

okwcmain2.o:	okwcmain2.C
okwcmain2.lo:	okwcmain2.C


CLEANFILES = core *.core *~ okwcmain2.C
EXTRA_DIST = .cvsignore okwcmain2.T
MAINTAINERCLEANFILES = Makefile.in

dist-hook:
	cd $(distdir) && rm -f okwcmain2.C
