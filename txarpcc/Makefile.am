## Process this file with automake to produce Makefile.in
# $Id$

LDADD = $(LIBSFSMISC) $(LIBARPC) $(LIBSFSCRYPT) $(LIBASYNC) $(LIBGMP) $(LIBPY)

okwsbin_PROGRAMS = txarpcc

noinst_HEADERS = rpcc.h parse.h ptxa.h

txarpcc_SOURCES = genheader.C gencfile.C genpython.C parse.yy scan.ll \
	rpcc.C ptxa.C

if STATIC
txarpcc_LDADD = $(LDADD)
txarpcc_DEPENDENCIES = $(LIBASYNC)
else
txarpcc_LDADD = $(LDADD)
txarpcc_DEPENDENCIES =
endif

AM_YFLAGS = -d

parse.h:
	@:

scan.cc: scan.ll
	$(LEX) $(AM_LFLAGS) $(LFLAGS) $(srcdir)/scan.ll \
		&& mv $(LEX_OUTPUT_ROOT).c $(srcdir)/scan.cc

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

#
#parse.o: parse.cc parse.h
#  
#  got rid of the dependcy on parse.h, which was confusing gmake
#
parse.o: parse.cc
# Note: certain versions of bison put __attribute__((unused)) after
# unused goto labels, which doesn't agree well with gcc 3.2.1.
# If you have problems, try the next line instead of the following one.
#	$(CXXCOMPILE) $(CXXNOERR) -D'__attribute__(x)=' -c $(srcdir)/parse.cc
	$(CXXCOMPILE) $(CXXNOERR) -c $(srcdir)/parse.cc

#scan.o: parse.o scan.cc parse.h
scan.o: parse.o scan.cc
	$(CXXCOMPILE) $(CXXNOERR) -c $(srcdir)/scan.cc

.PHONY: parseclean
parseclean:
	rm -f $(srcdir)/parse.cc $(srcdir)/parse.h $(srcdir)/scan.cc

BUILT_SOURCES = parse.cc scan.cc

CLEANFILES = core *.core *~ *.rpo

EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in parse.h parse.cc scan.cc
