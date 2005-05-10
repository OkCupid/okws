

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libahttp.la

libahttp_la_SOURCES = cgi.C ahttp.C err.C resp.C suiolite.C ahutil.C abuf.C \
	pair.C hdr.C inhdr.C ahparse.C aparse.C kmp.C mpfd.C

noinst_HEADERS = err.g

okwsinclude_HEADERS = cgi.h ahttp.h httpconst.h abuf.h pair.h hdr.h inhdr.h \
	aparse.h ahutil.h inhdr.h kmp.h mpfd.h pair.h resp.h recycle.h \
	suiolite.h ahparse.h

SUFFIXES = .g .C
.g.C:
	@rm -f $@
	-$(PUB) $(PUBFLAGS) -o $@ $< || rm -f $@

EXTRA_DIST = .cvsignore
CLEANFILES = core *.core *~ *.rpo

.PHONY: sourceclean
sourceclean:
	@rm -f $(srcdir)/err.C

MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 

BUILT_SOURCES = err.g
