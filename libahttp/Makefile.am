

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libahttp.la

libahttp_la_SOURCES = cgi.C ahttp.C err.C resp.C suiolite.C ahutil.C abuf.C \
	abuf_pipe.C pair.C hdr.C inhdr.C ahparse.C aparse.C kmp.C mpfd.C  \
	mimetypes.C httpconst.C

okwsinclude_HEADERS = okcgi.h ahttp.h httpconst.h abuf.h pair.h hdr.h inhdr.h \
	aparse.h ahutil.h inhdr.h kmp.h mpfd.h pair.h resp.h recycle.h \
	suiolite.h ahparse.h abuf_pipe.h mimetypes.h

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

resp.o: resp.C
resp.lo: resp.C

EXTRA_DIST = .cvsignore resp.T
CLEANFILES = core *.core *~ *.rpo resp.C

.PHONY: tameclean

tameclean:
	rm -f resp.C

dist-hook:
	cd $(distdir) && rm -f resp.C

MAINTAINERCLEANFILES = Makefile.in

