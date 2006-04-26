##
## $Id$
##
##   test2 is our attempt at regression and Unit tests; eventually, should
##   compile and run out of the box, and should be able to test itself.
##

$(PROGRAMS): $(LDEPS)

noinst_PROGRAMS = static configtest
static_SOURCES = static.C
configtest_SOURCES = configtest.C


SUFFIXES = .g .C .T
.T.C:
	-$(TAME) -c $< || rm -f $@

static.o:	static.C
static.lo:	static.C
configtest.o:	configtest.C
configtest.lo:	configtest.C

CLEANFILES = core *.core *~ *_config *_log
EXTRA_DIST = .cvsignore static.T okws_config.in pub_config.in configtest.T
BUILT_SOURCES = static.C configtest.C
MAINTAINERCLEANFILES = Makefile.in


.PHONY:	test tameclean

test:	okws_config pub_config 

okws_config: $(srcdir)/okws_config.in
	TOP_BUILD=`cd $(top_builddir) ; pwd` ;  \
	sed "s#@@TOP_BUILD@@#$$TOP_BUILD#g" < $< > $@

pub_config: $(srcdir)/pub_config.in
	TEST2_SRC=$(srcdir); \
	sed "s#@@TEST2_SRC@@#$$TEST2_SRC#g" < $< > $@

tameclean:
	rm -f $(srcdir)/static.C $(srcdir)/configtest.C

