
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libahttp.la

libahttp_la_SOURCES = cgi.C ahttp.C err.C resp.C suiolite.C ahutil.C abuf.C \
	abuf_pipe.C pair.C hdr.C inhdr.C ahparse.C aparse.C kmp.C mpfd.C  \
	mimetypes.C httpconst.C resp2.C ahttp2.C scratch.C

libahttp_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = okcgi.h ahttp.h httpconst.h abuf.h pair.h hdr.h inhdr.h \
	aparse.h ahutil.h inhdr.h kmp.h mpfd.h pair.h resp.h recycle.h \
	suiolite.h ahparse.h abuf_pipe.h mimetypes.h resp2.h okscratch.h

SUFFIXES = .T .g .C
.g.C:
	@rm -f $@
	-$(PUB) $(PUBFLAGS) -o $@ $< || rm -f $@

.T.C:
	-$(TAME) -o $@~ $< && mv $@~ $@ 

resp.o: resp.C
resp.lo: resp.C
resp2.o: resp.C
resp2.lo: resp.C
ahttp2.o: ahttp2.C
ahttp2.lo: ahttp2.C
err.o: err.C
err.lo: err.C

EXTRA_DIST = .cvsignore err.g resp.T resp2.T ahttp2.T
CLEANFILES = core *.core *~ *.rpo err.C resp.C resp2.C ahttp2.C

.PHONY: sourceclean tameclean
sourceclean:
	rm -f err.C

tameclean:
	rm -f resp.C resp2.C ahttp2.C

dist-hook:
	cd $(distdir) && rm -f err.C resp.C resp2.C ahttp2.C

MAINTAINERCLEANFILES = Makefile.in

