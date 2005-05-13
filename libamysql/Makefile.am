
$(PROGRAMS): $(LDEPS) 

okwslib_LTLIBRARIES = libamysql.la

libamysql_la_SOURCES = amysql_prot.C amysql.C mystmt_ag.C mystmt.C

mystmt_ag.C: mystmt.h

okwsinclude_HEADERS = amysql_prot.h amysql.h mystmt_ag.h mystmt.h

#noinst_HEADERS = mystmt_ag.pl amysql_prot.x

amysql.C: amysql_prot.h mystmt.h

amysql_prot.C: $(srcdir)/amysql_prot.x amysql_prot.h
	@rm -f $@
	-$(RPCC) -c $(srcdir)/amysql_prot.x || rm -f $@

amysql_prot.h: $(srcdir)/amysql_prot.x
	@rm -f $@
	-$(RPCC) -h $(srcdir)/amysql_prot.x || rm -f $@

mystmt_ag.h: $(srcdir)/mystmt_ag.pl
	@rm -f $@
	-$(PERL) $(srcdir)/mystmt_ag.pl -h > mystmt_ag.h || rm -f $@

mystmt_ag.C: $(srcdir)/mystmt_ag.pl
	@rm -f $@
	-$(PERL) $(srcdir)/mystmt_ag.pl -c > mystmt_ag.C || rm -f $@

EXTRA_DIST = .cvsignore mystmt_ag.pl amysql_prot.x
CLEANFILES = core *.core *~ *.rpo

MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 

BUILT_SOURCES = mystmt_ag.C mystmt_ag.h
