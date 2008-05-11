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

XML_SUBDIRS = xmlxlate

else

XMLPROGS =
NO_XML = \#

XML_SUBDIRS = 

endif

TAMEIN = configtest.T simple.T xmlex.T static.T form.T cookie.T upload.T
TAMEOUT = configtest.C simple.C xmlex.C static.C form.C cookie.C upload.C

SUBDIRS = $(XML_SUBDIRS)

okwssvc_PROGRAMS = static configtest simple form cookie upload $(XMLPROGS)

static_SOURCES = static.C
configtest_SOURCES = configtest.C
simple_SOURCES = simple.C
form_SOURCES = form.C
cookie_SOURCES = cookie.C
noinst_HEADERS = test_const.h
upload_SOURCS = upload.C

SUFFIXES = .g .C .T
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

static.o:	static.C
static.lo:	static.C
configtest.o:	configtest.C
configtest.lo:	configtest.C
simple.o:	simple.C
simple.lo:	simple.C
form.o:	    form.C
form.lo:    form.C
cookie.o:  cookie.C
cookie.lo: cookie.C
upload.o:	upload.C
uplaod.lo:	upload.C

CLEANFILES = core *.core *~ *_config *_log $(TAMEOUNT)
EXTRA_DIST = .cvsignore okws_config.in pub_config.in $(TAMEIN)
MAINTAINERCLEANFILES = Makefile.in

.PHONY:	test tameclean

test:	okws_config pub_config 

okws_config: $(srcdir)/okws_config.in
	TOP_BUILD=`cd $(top_builddir) ; pwd`  NO_XML=$(NO_XML)  \
	perl -ne ' { s#\@\@(.*?)\@\@#$$ENV{$$1}#e ; print $$_; } ' < $< > $@

pub_config: $(srcdir)/pub_config.in
	TOP_SRCDIR=`cd $(top_srcdir) ; pwd ` ; \
	sed "s#@@TOP_SRCDIR@@#$$TOP_SRCDIR#g" < $< > $@

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

