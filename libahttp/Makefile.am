

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libahttp.la

libahttp_la_SOURCES = cgi.C ahttp.C err.C resp.C suiolite.C ahutil.C abuf.C \
	abuf_pipe.C pair.C hdr.C inhdr.C ahparse.C aparse.C kmp.C mpfd.C  \
	mimetypes.C

okwsinclude_HEADERS = okcgi.h ahttp.h httpconst.h abuf.h pair.h hdr.h inhdr.h \
	aparse.h ahutil.h inhdr.h kmp.h mpfd.h pair.h resp.h recycle.h \
	suiolite.h ahparse.h abuf_pipe.h mimetypes.h

SUFFIXES = .T .g .C
.g.C:
	@rm -f $@
	-$(PUB) $(PUBFLAGS) -o $@ $< || rm -f $@

.T.C:
	-$(TAME) -o $@ $< || rm -f $@

resp.o: resp.C
resp.lo: resp.C
err.o: err.C
err.lo: err.C

EXTRA_DIST = .cvsignore err.g resp.T
CLEANFILES = core *.core *~ *.rpo err.C resp.C

.PHONY: sourceclean tameclean
sourceclean:
	rm -f err.C

tameclean:
	rm -f resp.C

dist-hook:
	cd $(distdir) && rm -f err.C resp.C

MAINTAINERCLEANFILES = Makefile.in

