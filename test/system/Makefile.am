##
## $Id$
##
##   test2 is our attempt at regression and Unit tests; eventually, should
##   compile and run out of the box, and should be able to test itself.
##

-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

if USE_XML

XMLPROGS = xmlex purify
NO_XML=

xmlex_SOURCES = xmlex.C
purify_SOURCES = purify.C purify_lib.C
XML_SUBDIRS = xmlxlate

else

XMLPROGS =
NO_XML = \#
XML_SUBDIRS = 

endif

if USE_SSL
NO_SSL=
else
NO_SSL = \#
endif

TAMEIN = configtest.T simple.T xmlex.T static.T form.T cookie.T \
	post.T upload.T purify.T purify_lib.T forloop.T reflect.T \
	objtest.T timer.T objtest2.T errortest.T slow.T cpubomb.T \
	double.T
TAMEOUT = configtest.C simple.C xmlex.C static.C form.C cookie.C \
	post.C upload.C purify.C purify_lib.C forloop.C reflect.C \
	objtest.C timer.C objtest2.C errortest.C slow.C cpubomb.C \
	double.C

SUBDIRS = $(XML_SUBDIRS) 3tier

okwssvc_PROGRAMS = static configtest simple form cookie \
	post upload $(XMLPROGS) posttest forloop reflect objtest \
	timer objtest2 errortest slow encoder cpubomb double

okwsconf_DATA = okws.crt.dist okws.key.dist

static_SOURCES = static.C okclib.C
configtest_SOURCES = configtest.C
simple_SOURCES = simple.C
double_SOURCES = double.C
form_SOURCES = form.C
post_SOURCES = post.C
cookie_SOURCES = cookie.C
noinst_HEADERS = test_const.h
upload_SOURCES = upload.C
posttest_SOURCES = posttest.C
forloop_SOURCES = forloop.C
reflect_SOURCES = reflect.C
objtest_SOURCES = objtest.C
objtest2_SOURCES = objtest2.C
timer_SOURCES = timer.C
errortest_SOURCES = errortest.C
slow_SOURCES = slow.C
encoder_SOURCES = encoder.C
cpubomb_SOURCES = cpubomb.C

SUFFIXES = .g .C .T
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

CLEANFILES = core *.core *~ *_config *_log $(TAMEOUT) 
EXTRA_DIST = .cvsignore okws_config.in pub_config.in $(TAMEIN) purify_lib.h \
	okws.crt.dist okws.key.dist
MAINTAINERCLEANFILES = Makefile.in

.PHONY:	test tameclean

test:	okws_config pub_config 

okws_config: $(srcdir)/okws_config.in
	TOP_BUILD=`cd $(top_builddir) ; pwd`  \
	NO_XML=$(NO_XML) \
	NO_SSL=$(NO_SSL) \
	TOP_SRCDIR=`cd $(top_srcdir) ; pwd` \
	perl -ne ' { s#\@\@(.*?)\@\@#$$ENV{$$1}#ge ; print $$_; } ' < $< > $@

pub_config: $(srcdir)/pub_config.in
	TOP_SRCDIR=`cd $(top_srcdir) ; pwd ` ; \
	sed "s#@@TOP_SRCDIR@@#$$TOP_SRCDIR#g" < $< > $@

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

