## Process this file with automake to produce Makefile.in
# $Id: Makefile.am 2263 2006-10-11 15:20:57Z max $

LDADD = $(LIBASYNC) $(LIBPY)

okwsbin_PROGRAMS = xmlrpcc

noinst_HEADERS = rpcc.h 

xmlrpcc_SOURCES = genheader.C gencfile.C parse.yy scan.ll rpcc.C 
xmlrpcc_LDADD = $(LDADD)
xmlrpcc_DEPENDENCIES = $(LIBASYNC)

YYFLAGS = -d

$(srcdir)/scan.cc: $(srcdir)/scan.ll
	$(LEX) $(AM_LFLAGS) $(LFLAGS) $< && mv $(LEX_OUTPUT_ROOT).c $@

$(srcdir)/parse.cc: $(srcdir)/parse.yy
	$(BISON) $(AM_YFLAGS) $(YYFLAGS) -o $@~ $<  \
	  && mv $@~ $@ \
	  && mv $(srcdir)/parse.hh~ $(srcdir)/parse.h

scan.lo: $(srcdir)/scan.cc parse.lo
	if $(LTCXXCOMPILE) $(CXXNOERR) -MT $@ -MD -MP \
	-MF "$(DEPDIR)/$*.Tpo" -c -o $@ $(srcdir)/scan.cc; \
	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Plo"; 

.PHONY: parseclean
parseclean:
	rm -f $(srcdir)/parse.cc $(srcdir)/parse.h $(srcdir)/scan.cc

BUILT_SOURCES = parse.cc parse.h scan.cc

CLEANFILES = core *.core *~ *.rpo

EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in parse.h parse.cc scan.cc
