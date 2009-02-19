## Process this file with automake to produce Makefile.in
# $Id$

-include $(SFS_LIB_MK)

LDADD = $(LIBSFSMISC) $(LIBSVC) $(LIBARPC) $(LIBSFSCRYPT) $(LIBASYNC) $(LDADD_THR_SFS) $(LIBGMP) $(LIBPY) $(LDADD_STD_ALL)

okwsbin_PROGRAMS = txarpcc

noinst_HEADERS = rpcc.h ptxa.h

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

$(srcdir)/scan.cc: $(srcdir)/scan.ll
	$(LEX) $(AM_LFLAGS) $(LFLAGS) $< && mv $(LEX_OUTPUT_ROOT).c $@

$(srcdir)/parse.cc: $(srcdir)/parse.yy
	$(BISON) $(AM_YFLAGS) $(YFLAGS) -o $@~ $<  \
	  && mv $@~ $@ \
	  && mv $(srcdir)/parse.hh~ $(srcdir)/parse.h

scan.o: parse.o $(srcdir)/scan.cc
	$(CXXCOMPILE) $(CXXNOERR) -c $(srcdir)/scan.cc

.PHONY: parseclean
parseclean:
	rm -f $(srcdir)/parse.cc $(srcdir)/parse.h $(srcdir)/scan.cc

BUILT_SOURCES = parse.cc scan.cc parse.h

CLEANFILES = core *.core *~ *.rpo

EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in parse.h parse.cc scan.cc
