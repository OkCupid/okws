##
## $Id$
##
##   test2 is our attempt at regression and Unit tests; eventually, should
##   compile and run out of the box, and should be able to test itself.
##

$(PROGRAMS): $(LDEPS)

if USE_XML

XMLPROGS = xmlex
NO_XML=

xmlex_SOURCES = xmlex.C
xmlex.o:	xmlex.C
xmlex.lo:	xmlex.C

else

XMLPROGS =
NO_XML = \#

endif

TAMEIN = configtest.T simple.T xmlex.T static.T
TAMEOUT = configtest.C simple.C xmlex.C static.C

okwssvc_PROGRAMS = static configtest simple $(XMLPROGS)
static_SOURCES = static.C
configtest_SOURCES = configtest.C
simple_SOURCES = simple.C

SUFFIXES = .g .C .T
.T.C:
	-$(TAME) -o $@ $< || rm -f $@

static.o:	static.C
static.lo:	static.C
configtest.o:	configtest.C
configtest.lo:	configtest.C
simple.o:	simple.C
simple.lo:	simple.C
xmlex.o:	xmlex.C
xmlex.lo:	xmlex.C

CLEANFILES = core *.core *~ *_config *_log $(TAMEOUNT)
EXTRA_DIST = .cvsignore okws_config.in pub_config.in $(TAMEIN)
MAINTAINERCLEANFILES = Makefile.in

.PHONY:	test tameclean

test:	okws_config pub_config 

okws_config: $(srcdir)/okws_config.in
	TOP_BUILD=`cd $(top_builddir) ; pwd`  NO_XML=$(NO_XML)  \
	perl -ne ' { s#\@\@(.*?)\@\@#$$ENV{$$1}#e ; print $$_; } ' < $< > $@

pub_config: $(srcdir)/pub_config.in
	TOP_SRCDIR=$(top_srcdir); \
	sed "s#@@TOP_SRCDIR@@#$$TOP_SRCDIR#g" < $< > $@

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

