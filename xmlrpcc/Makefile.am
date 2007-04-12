## Process this file with automake to produce Makefile.in
# $Id: Makefile.am 2263 2006-10-11 15:20:57Z max $

LDADD = $(LIBASYNC) $(LIBPY)

sfsbin_PROGRAMS = rpcc

noinst_HEADERS = rpcc.h parse.h

rpcc_SOURCES = genheader.C gencfile.C genpython.C parse.yy scan.ll rpcc.C \
	genpyc.C
if STATIC
rpcc_LDADD = $(LDADD) $(LDADD_THR)
rpcc_DEPENDENCIES = $(LIBASYNC)
else
rpcc_LDADD = $(LDADD) $(LDADD_THR)
rpcc_DEPENDENCIES =
endif

YYFLAGS = -d

parse.h:
	@:

scan.cc: scan.ll
	$(LEX) $(AM_LFLAGS) $(LFLAGS) $(srcdir)/scan.ll \
		&& mv $(LEX_OUTPUT_ROOT).c $(srcdir)/scan.cc

parse.cc: parse.yy
	rm -f y.tab.c
	$(YACC) $(AM_YFLAGS) $(YYFLAGS) $(srcdir)/parse.yy
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

parse.o: parse.cc 
# Note: certain versions of bison put __attribute__((unused)) after
# unused goto labels, which doesn't agree well with gcc 3.2.1.
# If you have problems, try the next line instead of the following one.
#	$(CXXCOMPILE) $(CXXNOERR) -D'__attribute__(x)=' -c $(srcdir)/parse.cc
	$(CXXCOMPILE) $(CXXNOERR) -c $(srcdir)/parse.cc

scan.o: parse.o scan.cc 
	$(CXXCOMPILE) $(CXXNOERR) -c $(srcdir)/scan.cc

.PHONY: parseclean
parseclean:
	rm -f $(srcdir)/parse.cc $(srcdir)/parse.h $(srcdir)/scan.cc

BUILT_SOURCES = parse.cc parse.h scan.cc

CLEANFILES = core *.core *~ *.rpo

EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in parse.h parse.cc scan.cc
