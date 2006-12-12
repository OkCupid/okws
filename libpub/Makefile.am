
$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libpub.la

libpub_la_SOURCES = xpub.C txa_prot.C data.C scan.ll parse.yy pubutil.C \
	pub.C xdr.C slave.C rclient.C proxy.C zstr.C okconst.C \
	pvalw.C jail.C fd_prot.C lbalance.C rpcwindow.C pub2.C clone.C \
	debug.C stats.C 

okwsinclude_HEADERS = pub.h pubutil.h xpub.h txa_prot.h okconst.h axprtfd.h \
	fhash.h fd_prot.h fhash.h holdtab.h lbalance.h parr.h \
	parse.h pjail.h pub_parse.h puberr.h zstr.h clist.h \
	pslave.h txa.h okdbg.h okdbg-int.h rpcwindow.h pub2.h okclone.h \
	okws_sfs.h timehash.h okstats.h 

noinst_HEADERS =  env.mk

YDEBUG = @YDEBUG@
LDEBUG = @LDEBUG@

SUFFIXES = .T .C .h .x

.T.C:
	-$(TAME) -o $@ $< || rm -f $@
.x.h:
	-$(RPCC) -h $< || rm -f $@
.x.C:
	-$(RPCC) -c $< || rm -f $@

TAMEIN = pub2.T clone.T
TAMEOUT = pub2.C clonce.C
RPCCIN = xpub.x fd_prot.x txa_prot.x
RPCCOUT = xpub.C xpub.h fd_prot.C fd_prot.h txa_prot.C txa_prot.h

pub2.o: pub2.C
pub2.lo: pub2.C
clone.o: clone.C
clone.lo: clone.C

xpub.h: xpub.x
xpub.C: xpub.x
xpub.o: xpub.h
xpub.lo: xpub.h

fd_prot.h: fd_prot.x
fd_prot.C: fd_prot.x
fd_prot.o: fd_prot.h
fd_prot.lo: fd_prot.h

txa_prot.h: txa_prot.x
txa_prot.C: txa_prot.x
txa_prot.o: txa_prot.h
txa_prot.lo: txa_prot.h

AM_YFLAGS = -d $(YDEBUG)
AM_LFLAGS = $(LDEBUG)

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

scan.lo: $(srcdir)/scan.cc parse.o
	if $(LTCXXCOMPILE) $(CXXNOERR) -MT $@ -MD -MP \
	-MF "$(DEPDIR)/$*.Tpo" -c -o $@ $(srcdir)/scan.cc; \
	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Plo"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi

.PHONY: parseclean
parseclean:
	rm -f $(srcdir)/parse.cc $(srcdir)/parse.h $(srcdir)/scan.cc

# Bison no longer generates bad code;  let the standard rules
# attempt to do the right thing
parse.o: parse.cc parse.h

#parse.o: parse.cc parse.h
# Note: certain versions of bison put __attribute__((unused)) after
# unused goto labels, which doesn't agree well with gcc 3.2.1.
# If you have problems, try the next line instead of the following one.
#	$(CXXCOMPILE) $(CXXNOERR) -D'__attribute__(x)=' -c $(srcdir)/parse.cc
#	$(CXXCOMPILE) $(CXXNOERR) -c $(srcdir)/parse.cc

.PHONY: xdrclean tameclean
xdrclean:
	rm -f $(RPCCOUT)

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(RPCCOUT) $(TAMEOUT)

.PHONY: sourceclean
sourceclean: parseclean xdrclean tameclean

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


BUILT_SOURCES = parse.cc parse.h scan.cc 
CLEANFILES = core *.core *~ *.rpo $(RPCCOUT) $(TAMEOUT)
EXTRA_DIST = .cvsignore $(RPCCIN) $(TAMEIN)
MAINTAINERCLEANFILES = Makefile.in parse.cc parse.h scan.cc
