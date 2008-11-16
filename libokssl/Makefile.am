

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libokssl.la
libokssl_la_SOURCES = proxy.C util.C sslcon.C
okwsinclude_HEADERS = oksslproxy.h oksslcon.h oksslutil.h

SUFFIXES = .C .T .h
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

TAMEIN = proxy.T sslcon.T
TAMEOUT = proxy.C slcon.C

proxy.o: proxy.C
proxy.lo: proxy.C
sslcon.o: sslcon.C
sslcon.lo: sslcon.C

.PHONY: tameclean

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

EXTRA_DIST = .cvsignore $(TAMEIN)
CLEANFILES = core *.core *~ *.rpo $(TAMEOUT)
MAINTAINERCLEANFILES = Makefile.in 
