

$(PROGRAMS): $(LDEPS)

noinst_HEADERS = okssl.h oksslproxy.h

okwsexec_PROGRAMS = okssld

okssld_SOURCES = proxy.T okssld.T util.C

SUFFIXES = .C .T .h

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

TAMEIN = proxy.T okssld.T
TAMEOUT = proxy.C okssld.C

okssld.o: okssld.C
okssld.lo: okssld.C
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
