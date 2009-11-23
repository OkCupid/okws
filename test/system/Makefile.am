##
## $Id$
##
##   test2 is our attempt at regression and Unit tests; eventually, should
##   compile and run out of the box, and should be able to test itself.
##

-include $(SFS_LIB_MK)

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

