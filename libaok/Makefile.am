
$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libaok.la

libaok_la_SOURCES = okprot.C ok.C log.C logbuf.C

okwsinclude_HEADERS = ok.h okprot.h oklog.h svq.h okerr.h

okprot.h: $(srcdir)/okprot.x
	@rm -f $@
	-$(RPCC) -h $(srcdir)/okprot.x || rm -f $@

okprot.C : $(srcdir)/okprot.x okprot.h
	@rm -f $@
	-$(RPCC) -c $(srcdir)/okprot.x || rm -f $@


EXTRA_DIST = .cvsignore
CLEANFILES = core *.core *~ *.rpo

MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 

BUILT_SOURCES = okprot.C okprot.h
