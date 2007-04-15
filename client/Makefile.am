
$(PROGRAMS): $(LDEPS)

noinst_PROGRAMS = hcli hsrv degz mygz pt1cli okwc tmr bench mygz2 enctst okwc3
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
okwc3_SOURCES = okwcmain3.C
mygz2_SOURCES = mygz2.C

okdbg_LDADD = $(LIBASYNC) $(LDADD_THR_SFS) $(LIBPY)

TAMEOUT = okwcmain3.C

SUFFIXES = .C .T .h

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

okwcmain3.o:	okwcmain3.C
okwcmain3.lo:	okwcmain3.C


CLEANFILES = core *.core *~ $(TAMEOUT)
EXTRA_DIST = .cvsignore okwcmain3.T
MAINTAINERCLEANFILES = Makefile.in

.PHONY: tameclean

tameclean:
	rm -rf $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)
