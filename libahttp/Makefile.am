

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libahttp.la

libahttp_la_SOURCES = cgi.C ahttp.C err.C resp.C suiolite.C ahutil.C abuf.C \
	abuf_pipe.C pair.C hdr.C inhdr.C ahparse.C aparse.C kmp.C mpfd.C \
	xmlparse.C xmldata.C xmlwrap.C

# noinst_HEADERS = err.g

okwsinclude_HEADERS = okcgi.h ahttp.h httpconst.h abuf.h pair.h hdr.h inhdr.h \
	aparse.h ahutil.h inhdr.h kmp.h mpfd.h pair.h resp.h recycle.h \
	suiolite.h ahparse.h okxml.h okxmldata.h abuf_pipe.h smartvec.h \
	okxmlwrap.h

SUFFIXES = .T .g .C
.g.C:
	@rm -f $@
	-$(PUB) $(PUBFLAGS) -o $@ $< || rm -f $@
.T.C:
	-$(TAME) -c $< || rm -f $@

resp.o: resp.C
resp.lo: resp.C

EXTRA_DIST = .cvsignore err.g resp.T
CLEANFILES = core *.core *~ *.rpo

.PHONY: sourceclean
sourceclean:
	@rm -f $(srcdir)/err.C

tameclean:
	rm -f $(srcdir)/resp.C 

MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 

BUILT_SOURCES = err.g resp.C
