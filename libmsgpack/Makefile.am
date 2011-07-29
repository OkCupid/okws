
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libmsgpack.la

libmsgpack_la_SOURCES = \
	msgpack.C

libmsgpack_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = \
	okmsgpack.h

YDEBUG = @YDEBUG@
LDEBUG = @LDEBUG@

SUFFIXES = .T .C .h

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

TAMEIN = 
TAMEOUT = 

.PHONY: tameclean
tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(RPCCOUT) $(TAMEOUT)

.PHONY: sourceclean
sourceclean: parseclean xdrclean tameclean

BUILT_SOURCES = 
CLEANFILES = core *.core *~ *.rpo $(RPCCOUT) $(TAMEOUT)
EXTRA_DIST = .cvsignore $(RPCCIN) $(TAMEIN)
MAINTAINERCLEANFILES = Makefile.in 
