
$(PROGRAMS): $(LDEPS)

noinst_PROGRAMS = hcli degz mygz pt1cli okwc tmr bench mygz2 enctst okwc3 okwc4
okwsbin_PROGRAMS = okdbg

noinst_HEADERS = normdist.h

okdbg_SOURCES = okdbg.C
hcli_SOURCES = hcli.C
okwc_SOURCES = okwcmain.C
pt1cli_SOURCES = pt1cli.C normdist.C
degz_SOURCES = degz.C 
mygz_SOURCES = mygz.C
tmr_SOURCES = tmr.C
bench_SOURCES = bench.C
enctst_SOURCES = enctst.C
okwc3_SOURCES = okwcmain3.C
okwc4_SOURCES = okwcmain4.C
mygz2_SOURCES = mygz2.C


TAMEOUT = okwcmain3.C okwcmain4.C

SUFFIXES = .C .T .h

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

okwcmain3.o:	okwcmain3.C
okwcmain3.lo:	okwcmain3.C
okwcmain4.o: 	okwcmain4.C
okwcmain4.o:	okwcmain4.C


CLEANFILES = core *.core *~ $(TAMEOUT)
EXTRA_DIST = .cvsignore okwcmain3.T okwcmain4.T
MAINTAINERCLEANFILES = Makefile.in

.PHONY: tameclean

tameclean:
	rm -rf $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)
