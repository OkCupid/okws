## Process this file with automake to produce Makefile.in
# $Id$

LDADD = $(LIBSFSMISC) $(LIBSVC) $(LIBARPC) $(LIBSFSCRYPT) $(LIBASYNC) $(LDADD_THR_SFS) $(LIBGMP) $(LIBPY)

okwsbin_PROGRAMS = txarpcc

noinst_HEADERS = rpcc.h parse.h ptxa.h

txarpcc_SOURCES = genheader.C gencfile.C genpython.C parse.yy scan.ll \
	rpcc.C ptxa.C

if STATIC
txarpcc_LDADD = $(LDADD)
txarpcc_DEPENDENCIES = $(LIBASYNC)
else
txarpcc_LDADD = $(LDADD)
txarpcc_DEPENDENCIES =
endif

AM_YFLAGS = -d

parse.h:
	@:

$(srcdir)/scan.cc: $(srcdir)/scan.ll
	$(LEX) $(AM_LFLAGS) $(LFLAGS) $< && mv $(LEX_OUTPUT_ROOT).c $@

$(srcdir)/parse.cc: $(srcdir)/parse.yy
	$(BISON) $(AM_YFLAGS) $(YFLAGS) -o $@~ $<  \
	  && mv $@~ $@ \
	  && mv $(srcdir)/parse.hh~ $(srcdir)/parse.h

scan.lo: $(srcdir)/scan.cc parse.lo
	if $(LTCXXCOMPILE) $(CXXNOERR) -MT $@ -MD -MP \
	-MF "$(DEPDIR)/$*.Tpo" -c -o $@ $(srcdir)/scan.cc; \
	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Plo"; 

.PHONY: parseclean
parseclean:
	rm -f $(srcdir)/parse.cc $(srcdir)/parse.h $(srcdir)/scan.cc

BUILT_SOURCES = parse.cc scan.cc

CLEANFILES = core *.core *~ *.rpo

EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in parse.h parse.cc scan.cc
