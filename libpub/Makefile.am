
$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libpub.la

libpub_la_SOURCES = xpub.C txa_prot.C data.C scan.ll parse.yy pubutil.C \
	pub.C xdr.C slave.C rclient.C proxy.C zstr.C okconst.C \
	pvalw.C jail.C fd_prot.C lbalance.C rpcwindow.C stats.C

okwsinclude_HEADERS = pub.h pubutil.h xpub.h txa_prot.h okconst.h axprtfd.h \
	fhash.h fd_prot.h fhash.h holdtab.h lbalance.h parr.h \
	parse.h pjail.h pub_parse.h puberr.h zstr.h clist.h \
	pslave.h txa.h okdbg.h okdbg-int.h rpcwindow.h okws_sfs.h okstats.h

noinst_HEADERS =  xpub.x fd_prot.x txa_prot.x env.mk

YDEBUG = @YDEBUG@
LDEBUG = @LDEBUG@

AM_YFLAGS = -d $(YDEBUG)
AM_LFLAGS = $(LDEBUG)

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

txa_prot.h:	$(srcdir)/txa_prot.x
	@rm -f $@
	-$(RPCC) -h $(srcdir)/txa_prot.x || rm -f $@

txa_prot.C:	$(srcdir)/txa_prot.x txa_prot.h
	@rm -f $@
	-$(RPCC) -c $(srcdir)/txa_prot.x || rm -f $@


scan.cc: scan.ll
	$(LEX) $(AM_LFLAGS) $(LFLAGS) $(srcdir)/scan.ll 		\
	&& mv $(LEX_OUTPUT_ROOT).c $(srcdir)/scan.cc

parse.h:
	@:
parse.cc: parse.yy
	rm -f y.tab.c
	$(YACC) $(AM_YFLAGS) $(YFLAGS) $(srcdir)/parse.yy
	case "$(YACC)" in					\
	*bison*)						\
		sed -e 's/union yyalloc/struct yyalloc/g' 	\
			< y.tab.c |				\
		sed -e 's/__attribute__ \(\(.*\)\)/if (0) YYERROR;/'  \
			> $(srcdir)/parse.cc ;			\
		rm -f y.tab.c 					\
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

parse.o: parse.cc parse.h
# Note: certain versions of bison put __attribute__((unused)) after
# unused goto labels, which doesn't agree well with gcc 3.2.1.
# If you have problems, try the next line instead of the following one.
#	$(CXXCOMPILE) $(CXXNOERR) -D'__attribute__(x)=' -c $(srcdir)/parse.cc
	$(CXXCOMPILE) $(CXXNOERR) -c $(srcdir)/parse.cc

.PHONY: xdrclean
xdrclean:
	rm -f $(srcdir)/xpub.C $(srcdir)/xpub.h
	rm -f $(srcdir)/fd_prot.C $(srcdir)/fd_prot.h
	rm -f $(srcdir)/txa_prot.C $(srcdir)/txa_prot.h

.PHONY: sourceclean
sourceclean: parseclean xdrclean

#
# include Makefile flags and options for pysfs1 
#
env.mk: Makefile
	@rm -f env.mk~
	echo "# Makefile fragment generated " `date` > env.mk~
	echo "CC = $(CC)" >> env.mk~
	echo "CXX = $(CXX)" >> env.mk~
	echo "CFLAGS = $(CFLAGS)" >> env.mk~
	echo "CXXFLAGS = $(CXXFLAGS)" >> env.mk~
	echo "LIBS = $(LIBS)" >> env.mk~
	mv -f env.mk~ env.mk


BUILT_SOURCES = parse.cc parse.h scan.cc xpub.C xpub.h fd_prot.C fd_prot.h \
	txa_prot.h txa_prot.C

CLEANFILES = core *.core *~ *.rpo 
EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in  parse.cc parse.h scan.cc
