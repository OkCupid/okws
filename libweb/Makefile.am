
$(PROGRAMS): $(LDEPS) 

sfslib_LTLIBRARIES = libweb.la

libweb_la_SOURCES = web_prot.C form.C inputcheck.C web.C email.C okwc.C

sfsinclude_HEADERS = web_prot.h web.h form.h email.h okwc.h

web_prot.C: $(srcdir)/web_prot.x web_prot.h
	@rm -f $@
	-$(RPCC) -c $(srcdir)/web_prot.x || rm -f $@

web_prot.h: $(srcdir)/web_prot.x
	@rm -f $@
	-$(RPCC) -h $(srcdir)/web_prot.x || rm -f $@

EXTRA_DIST = .cvsignore
CLEANFILES = core *.core *~ *.rpo

MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 
