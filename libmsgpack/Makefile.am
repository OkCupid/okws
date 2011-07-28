
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libmsgpack.la

libpub_la_SOURCES = \
	msgpack.C

libpub_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = \
	okmsgpack.h

YDEBUG = @YDEBUG@
LDEBUG = @LDEBUG@

SUFFIXES = .T .C .h .x

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)
.x.h:
	-$(RPCC) -h $< || rm -f $@
.x.C:
	-$(RPCC) -c $< || rm -f $@

TAMEIN = 
TAMEOUT = 
RPCCIN = 
RPCCOUT =

.PHONY: parseclean
parseclean:
	rm -f $(srcdir)/parse.cc $(srcdir)/parse.h $(srcdir)/scan.cc

.PHONY: xdrclean tameclean
xdrclean:
	rm -f $(RPCCOUT)

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
