
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS) 

okwslib_LTLIBRARIES = libamysql.la
libamysql_la_SOURCES = amysql.C mystmt_ag.C mystmt.C tz.C mybind.C gmtoff.C
okwsinclude_HEADERS = amysql.h mystmt_ag.h mystmt.h mybind.h gmtoff.h

libamysql_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

mystmt_ag.C: mystmt_ag.h
mystmt_ag.o: mystmt_ag.C
mystmt_ag.lo: mystmt_ag.C
amysql.o: mystmt_ag.h amysql.C
amysql.lo: mystmt_ag.h amysql.C
mystmt.o: mystmt_ag.h
mystmt.lo: mystmt_ag.h

mystmt_ag.h: $(srcdir)/mystmt_ag.pl
	@rm -f $@
	-$(PERL) $< -h > $@~ && mv $@~ $@
mystmt_ag.C: $(srcdir)/mystmt_ag.pl
	@rm -f $@
	-$(PERL) $< -c > $@~ && mv $@~ $@

PERLOUT = mystmt_ag.C mystmt_ag.h
PERLIN = mystmt_ag.pl

EXTRA_DIST = $(PERLIN)
CLEANFILES = core *.core *~ *.rpo $(PERLOUT)

MAINTAINERCLEANFILES = Makefile.in

dist-hook:
	cd $(distdir) && rm -f $(PERLOUT)

