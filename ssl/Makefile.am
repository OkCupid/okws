

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libokssl.la
noinst_PROGRAMS = ssl-proxy


libokssl_la_SOURCES = okssl.T


SUFFIXES = .C .T .h

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

ssl_proxy_SOURCES = proxy.C

TAMEIN = proxy.T okssl.T
TAMEOUT = proxy.C okssl.C

okssl.o: okssl.C
okssl.lo: okssl.C
proxy.o: proxy.C
proxy.lo: proxy.C

.PHONY: tameclean

tameclean:
	rm -f $(TAMEOUT)


dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

EXTRA_DIST = .cvsignore $(TAMEIN)
CLEANFILES = core *.core *~ *.rpo $(TAMEOUT)
MAINTAINERCLEANFILES = Makefile.in 
