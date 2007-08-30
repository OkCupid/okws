
$(PROGRAMS): $(LDEPS) 

okwslib_LTLIBRARIES = libamysql.la
libamysql_la_SOURCES =  amysql.C mystmt_ag.C mystmt.C
okwsinclude_HEADERS = amysql.h mystmt_ag.h mystmt.h

mystmt_ag.C: mystmt_ag.h
mystmt_ag.o: mystmt_ag.C
mystmt_ag.lo: mystmt_ag.C
amysql.o: mystmt_ag.h
amysql.lo: mystmt_ag.h
mystmt.o: mystmt_ag.h
mystmt.lo: mystmt_ag.h


mystmt_ag.h: $(srcdir)/mystmt_ag.pl
	@rm -f $@
	-$(PERL) $(srcdir)/mystmt_ag.pl -h > mystmt_ag.h || rm -f $@

mystmt_ag.C: $(srcdir)/mystmt_ag.pl
	@rm -f $@
	-$(PERL) $(srcdir)/mystmt_ag.pl -c > mystmt_ag.C || rm -f $@

amysql.C: mystmt_ag.h 

PERLOUT = mystmt_ag.C mystmt_ag.h
PERLIN = mystmt_ag.pl

EXTRA_DIST = .cvsignore $(PERLIN)
CLEANFILES = core *.core *~ *.rpo $(PERLOUT)

MAINTAINERCLEANFILES = Makefile.in

dist-hook:
	cd $(distdir) && rm -f $(PERLOUT)

