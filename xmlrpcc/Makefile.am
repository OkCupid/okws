## Process this file with automake to produce Makefile.in
# $Id: Makefile.am 2263 2006-10-11 15:20:57Z max $

-include $(SFS_LIB_MK)

LDADD = $(LIBASYNC) $(LIBPY) $(LDADD_STD_ALL)

okwsbin_PROGRAMS = xmlrpcc

noinst_HEADERS = rpcc.h parse.h

xmlrpcc_SOURCES = genheader.C gencfile.C parse.yy scan.ll rpcc.C 
xmlrpcc_LDADD = $(LDADD)
xmlrpcc_DEPENDENCIES = $(LIBASYNC)

YYFLAGS = -d

parse.h:
	@:

unexport POSIXLY_CORRECT
$(srcdir)/scan.cc: $(srcdir)/scan.ll
	$(LEX) $(AM_LFLAGS) $(LFLAGS) $< && mv $(LEX_OUTPUT_ROOT).c $@

$(srcdir)/parse.cc: $(srcdir)/parse.yy
	$(BISON) $(AM_YFLAGS) $(YYFLAGS) -o $@~ $<  \
	  && mv $@~ $@ \
	  && mv $(srcdir)/parse.hh~ $(srcdir)/parse.h

scan.o: parse.o $(srcdir)/scan.cc $(srcdir)/parse.cc
	$(CXXCOMPILE) $(CXXNOERR) -c $(srcdir)/scan.cc

.PHONY: parseclean
parseclean:
	rm -f $(srcdir)/parse.cc $(srcdir)/parse.h $(srcdir)/scan.cc

BUILT_SOURCES = parse.cc parse.h scan.cc

CLEANFILES = core *.core *~ *.rpo

EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in parse.h parse.cc scan.cc
