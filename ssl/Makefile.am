

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libokssl.la
libokssl_la_SOURCES = proxy.C util.C sslcon.C
okwsinclude_HEADERS = oksslproxy.h oksslcon.h oksslutil.h

SUFFIXES = .C .T .h
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

TAMEIN = proxy.T okssld.T sslcon.T
TAMEOUT = proxy.C okssld.C sslcon.C

proxy.o: proxy.C
proxy.lo: proxy.C
sslcon.o: sslcon.C
sslcon.lo: sslcon.C

if USE_SSL
okwsexec_PROGRAMS = okssld
okssld_SOURCES = okssld.C
okssld.o: okssld.C
okssld.lo: okssld.C
endif

.PHONY: tameclean

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

EXTRA_DIST = .cvsignore $(TAMEIN)
CLEANFILES = core *.core *~ *.rpo $(TAMEOUT)
MAINTAINERCLEANFILES = Makefile.in 
