
$(PROGRAMS): $(LDEPS) 

okwslib_LTLIBRARIES = libweb.la

libweb_la_SOURCES = web_prot.C form.C inputcheck.C web.C email.C okwc.C \
	okwc2.C okwcxml.C

okwsinclude_HEADERS = web_prot.h web.h form.h email.h okwc.h okwc2.h \
	okwcxml.h

SUFFIXES = .C .T .h .x

.T.C:
	-$(TAME) -o $@ $< || rm -f $@
.x.h:
	-$(RPCC) -h $< || rm -f $@
.x.C:
	-$(RPCC) -c $< || rm -f $@

TAMEIN = okwc2.T okwcxml.T
TAMEOUT = okwc2.C okwcxml.C
RPCCOUT = web_prot.h web_prot.C

okwc2.o:	okwc2.C
okwc2.lo:	okwc2.C
okwcxml.o:	okwcxml.C
okwcxml.lo:	okwcxml.C

web_prot.h:	web_prot.x
web_prot.C:	web_prot.C
web_prot.o:	web_prot.h
web_prot.lo:	web_prot.h

.PHONY: xdrclean tameclean sourceclean

xdrclean:
	rm -f $(RPCCOUT)

tameclean:
	rm -f $(TAMEOUT)

soureclean: xdrclean tameclean

EXTRA_DIST = .cvsignore $(TAMEIN) web_prot.x
CLEANFILES = core *.core *~ *.rpo $(TAMEOUT) $(RPCCOUT)
MAINTAINERCLEANFILES = Makefile.in


dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT) $(RPCCOUT)
