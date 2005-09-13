
$(PROGRAMS): $(LDEPS) 

okwslib_LTLIBRARIES = libamysql.la

libamysql_la_SOURCES = amysql.C mystmt_ag.C mystmt.C

mystmt_ag.C: mystmt.h

okwsinclude_HEADERS = amysql.h mystmt_ag.h mystmt.h

amysql.C: mystmt.h


mystmt_ag.h: $(srcdir)/mystmt_ag.pl
	@rm -f $@
	-$(PERL) $(srcdir)/mystmt_ag.pl -h > mystmt_ag.h || rm -f $@

mystmt_ag.C: $(srcdir)/mystmt_ag.pl
	@rm -f $@
	-$(PERL) $(srcdir)/mystmt_ag.pl -c > mystmt_ag.C || rm -f $@

EXTRA_DIST = .cvsignore mystmt_ag.pl
CLEANFILES = core *.core *~ *.rpo

MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 

BUILT_SOURCES = mystmt_ag.C mystmt_ag.h
