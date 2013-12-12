
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libahttp.la

libahttp_la_SOURCES = cgi.C ahttp.C err.C resp.C suiolite.C ahutil.C abuf.C \
	abuf_pipe.C pair.C hdr.C inhdr.C ahparse.C aparse.C kmp.C mpfd.C  \
	mimetypes.C httpconst.C resp2.C ahttp2.C scratch.C

libahttp_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = okcgi.h ahttp.h httpconst.h abuf.h hdr.h \
	aparse.h ahutil.h inhdr.h kmp.h mpfd.h pair.h resp.h recycle.h \
	suiolite.h ahparse.h abuf_pipe.h mimetypes.h resp2.h okscratch.h

SUFFIXES = .T .C

.T.C:
	-$(TAME) -o $@~ $< && mv $@~ $@ 

resp.o: resp.C
resp.lo: resp.C
resp2.o: resp.C
resp2.lo: resp.C
ahttp2.o: ahttp2.C
ahttp2.lo: ahttp2.C

EXTRA_DIST = .cvsignore resp.T resp2.T ahttp2.T ahparse.T
CLEANFILES = core *.core *~ *.rpo resp.C resp2.C ahttp2.C ahparse.C

.PHONY: tameclean

tameclean:
	rm -f resp.C resp2.C ahttp2.C ahparse.C

dist-hook:
	cd $(distdir) && rm -f resp.C resp2.C ahttp2.C ahparse.C

MAINTAINERCLEANFILES = Makefile.in

