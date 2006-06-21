
$(PROGRAMS): $(LDEPS) 

okwslib_LTLIBRARIES = libweb.la

libweb_la_SOURCES = web_prot.C form.C inputcheck.C web.C email.C okwc.C \
	okwc2.C

okwsinclude_HEADERS = web_prot.h web.h form.h email.h okwc.h okwc2.h

# cannot install, for now, since some broken Makefile.am's suck this
# in and recompile, triggering a whole recompile
noinst_HEADERS = web_prot.x

SUFFIXES = .C .T .h

.T.C:
	-$(TAME) -c $< || rm -f $@

okwc2.o:	okwc2.C
okwc2.lo:	okwc2.C

web_prot.C: $(srcdir)/web_prot.x web_prot.h
	@rm -f $@
	-$(RPCC) -c $(srcdir)/web_prot.x || rm -f $@

web_prot.h: $(srcdir)/web_prot.x
	@rm -f $@
	-$(RPCC) -h $(srcdir)/web_prot.x || rm -f $@

.PHONY: xdrclean tameclean sourceclean

xdrclean:
	rm -f $(srcdir)/web_prot.h $(src_dir)/webprot.C

tameclean:
	rm -f $(srcdir)/okwc2.C

soureclean: xdrclean tameclean

EXTRA_DIST = .cvsignore okwc2.T

BUILT_SOURCES = web_prot.h web_prot.C okwc2.C
CLEANFILES = core *.core *~ *.rpo $(BUILT_SOURCES)

MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 
