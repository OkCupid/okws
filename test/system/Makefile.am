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
xmlex.o:	xmlex.C
xmlex.lo:	xmlex.C
purify.o:	purify.C
purify.lo:	purify.C
purify_lib.o:	purify_lib.C
purify_lib.lo:	purify_lib.C

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

TAMEIN = configtest.T simple.T xmlex.T static.T pub1tst.T form.T cookie.T \
	post.T upload.T purify.T purify_lib.T forloop.T pub3i64.T
TAMEOUT = configtest.C simple.C xmlex.C static.C pub1tst.C form.C cookie.C \
	post.C upload.C purify.C purify_lib.C forloop.C pub3i64.C

SUBDIRS = $(XML_SUBDIRS) 3tier

okwssvc_PROGRAMS = static configtest simple pub1tst form cookie \
	post upload $(XMLPROGS) posttest forloop pub3i64

okwsconf_DATA = okws.crt.dist okws.key.dist

static_SOURCES = static.C
configtest_SOURCES = configtest.C
simple_SOURCES = simple.C
pub1tst_SOURCES = pub1tst.C
form_SOURCES = form.C
post_SOURCES = post.C
cookie_SOURCES = cookie.C
noinst_HEADERS = test_const.h
upload_SOURCES = upload.C
posttest_SOURCES = posttest.C
forloop_SOURCES = forloop.C
pub3i64_SOURCES = pub3i64.C

SUFFIXES = .g .C .T
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

static.o:	static.C
static.lo:	static.C
configtest.o:	configtest.C
configtest.lo:	configtest.C
simple.o:	simple.C
simple.lo:	simple.C
pub1tst.o: pub1tst.C
pub1tst.lo: pub1tst.C
form.o:	    form.C
form.lo:    form.C
post.o:	   post.C
post.lo:   post.C
cookie.o:  cookie.C
cookie.lo: cookie.C
upload.o:	upload.C
upload.lo:	upload.C
forloop.lo:	forloop.C
forloop.o:	forloop.C
pub3i64.loo:      pub3i64.C
pub3i64.o:      pub3i64.C

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

