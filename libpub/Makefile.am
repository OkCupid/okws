
$(PROGRAMS): $(LDEPS)

sfslib_LTLIBRARIES = libpub.la

libpub_la_SOURCES = xpub.C data.C scan.ll parse.yy pubutil.C pub.C xdr.C \
	slave.C rclient.C proxy.C zstr.C okconst.C pvalw.C jail.C fd_prot.C \
	authtok.C

sfsinclude_HEADERS = pub.h pubutil.h xpub.h

YDEBUG = @YDEBUG@
LDEEBUG = @LDEBUG@

YFLAGS = -d $(YDEBUG)
LFLAGS = $(LDEBUG)

xpub.C:	$(srcdir)/xpub.x xpub.h
	@rm -f $@
	-$(RPCC) -c $(srcdir)/xpub.x || rm -f $@

xpub.h:	$(srcdir)/xpub.x
	@rm -f $@
	-$(RPCC) -h $(srcdir)/xpub.x || rm -f $@

fd_prot.C:	$(srcdir)/fd_prot.x fd_prot.h
	@rm -f $@
	-$(RPCC) -c $(srcdir)/fd_prot.x || rm -f $@

fd_prot.h:	$(srcdir)/fd_prot.x
	@rm -f $@
	-$(RPCC) -h $(srcdir)/fd_prot.x || rm -f $@

scan.cc: scan.ll
	$(LEX) $(AM_FLAGS) $(LFLAGS) $(srcdir)/scan.ll 		\
	&& mv $(LEX_OUTPUT_ROOT).c $(srcdir)/scan.cc

parse.h:
	@:
parse.cc: parse.yy
	rm -f y.tab.c
	$(YACC) $(AM_YFLAGS) $(YFLAGS) $(srcdir)/parse.yy
	case "$(YACC)" in					\
	*bison*)						\
		sed -e 's/union yyalloc/struct yyalloc/g'	\
			< y.tab.c > $(srcdir)/parse.cc ;	\
		rm -f y.tab.c					\
		;;						\
	*)							\
		mv -f y.tab.c $(srcdir)/parse.cc		\
		;;						\
	esac
	if test -f y.tab.h; then				\
		if cmp -s y.tab.h $(srcdir)/parse.h; then	\
			rm -f y.tab.h;				\
		else						\
			mv y.tab.h $(srcdir)/parse.h;		\
		fi;						\
	fi

scan.o: parse.o scan.cc parse.h
	$(CXXCOMPILE) $(CXXNOERR) -c $(srcdir)/scan.cc

.PHONY: parseclean
parseclean:
	rm -f $(srcdir)/parse.cc $(srcdir)/parse.h $(srcdir)/scan.cc

.PHONY: xdrclean
xdrclean:
	rm -f $(srcdir)/xpub.C $(srcdir)/xpub.h

.PHONY: sourceclean
sourceclean: parseclean xdrclean


BUILT_SOURCES = parse.cc parse.h scan.cc xpub.C xpub.h

CLEANFILES = core *.core *~ *.rpo 
EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 
