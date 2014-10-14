## Process this file with automake to produce Makefile.in
## $Id$

include $(SFS_LIB_MK)

if USE_PTHREADS
PTHR = libamt_pthread
endif

SBD1 = \
	libpub \
	xmlrpcc \
	libahttp \
	libokxml \
	libaok \
	libamt \
	$(PTHR) \
	libokssl \
	libweb \
	librfn \
	pub

if USE_MYSQL
SBD2 = libamysql ezdb
endif

if HAVE_PYTHON
SBD3 = py
endif

if USE_REDIS
SBD4 = libredis
endif

SUBDIRS = $(SBD1) \
	$(SBD2) \
	okd \
	ssld \
	logd \
	client \
	test \
	conf \
	htdocs \
	$(SBD3) \
	$(SBD4) \
	contrib

##
## New aclocal (1.10+) is asking for this...
##
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = setup autoconf.sed .cvsignore setup.gnu acokws.m4 \
	build/flex_vers.pl

okwsbuildtools_DATA = acokws.m4

CLEANFILES = core *.core *~

MAINTAINERCLEANFILES = 				\
	aclocal.m4 install-sh mkinstalldirs missing depcomp \
	configure config.guess config.sub config.h.in \
	ltconfig ltmain.sh \
	INSTALL COPYING Makefile.in stamp-h.in

okwsinclude_HEADERS = okwsconf.h
BUILT_SOURCES = okwsconf.h
DISTCLEANFILES = okwsconf.h stamp-auto-h autom4te*.cache
all-recursive install-recursive: okwsconf.h

okwsconf.h: stamp-auto-h
	@:

stamp-auto-h: $(srcdir)/autoconf.sed $(CONFIG_HEADER)
	@rm -f autoconf.h~
	sed -f $(srcdir)/autoconf.sed $(CONFIG_HEADER) > autoconf.h~
	cmp autoconf.h~ okwsconf.h > /dev/null 2>& 1 \
	  || mv -f autoconf.h~ okwsconf.h
	@rm -f autoconf.h~
	@echo timepstamp > stamp-auto-h

dist-hook:
	cd $(distdir) && rm -f okwsconf.h
	cd $(distdir) && chmod -x setup

install-data-local:
	-src=$(DESTDIR)$(okwsincludedirp); dst=$(DESTDIR)$(pkgincludedir); \
	rm -f $$dst; \
	if test "`dirname $$src`" = "`dirname $$dst`"; then \
		ln -s "`basename $$src`" $$dst; \
	else \
		ln -s $(okwsincludedirp) $$dst; \
	fi

install-exec-local:
	-src=$(DESTDIR)$(okwslibdirp); dst=$(DESTDIR)$(pkglibdir); \
	rm -f $$dst; \
	if test "`dirname $$src`" = "`dirname $$dst`"; then \
		ln -s "`basename $$src`" $$dst; \
	else \
		ln -s $(okwslibdirp) $$dst; \
	fi

install-data-hook:
	$(INSTALL_DATA) libpub/env.mk $(DESTDIR)$(okwslibdir)

uninstall-local:
	-rm -f $(DESTDIR)$(pkglibdir) $(DESTDIR)$(pkgincludedir)
	-rmdir $(DESTDIR)$(okwslibdir) $(DESTDIR)$(owksincludedir) \
		$(DESTDIR)$(confdir)

all-recursive-am: aux-all
#mostlyclean-recursive: aux-mostlyclean
#clean-recursive: aux-clean
distclean-recursive: aux-distclean
maintainer-clean-recursive: aux-maintainer-clean

AUX_TARGET = `echo $@ | sed -e 's/^aux-//'`
.PHONY: aux-all aux-mostlyclean aux-clean aux-distclean aux-maintainer-clean
aux-all aux-mostlyclean aux-clean aux-distclean aux-maintainer-clean:
	test -z "$(GMP_DIR)" || (cd "$(GMP_DIR)" && $(MAKE) $(AUX_TARGET))
	test -z "$(DB3_DIR)" -o "$(DB3_DIR)" = "@"DB3_DIR"@" \
		|| (cd "$(DB3_DIR)/dist" && $(MAKE) $(AUX_TARGET))

.PHONY: rpcclean
rpcclean:
	for dir in libpub libweb libamysql libaok; do \
		(cd $$dir && $(MAKE) rpcclean); \
	done

.PHONY: tameclean
tameclean:
	for dir in libpub libahttp libaok libweb okd pub client test ; do \
		(cd $$dir && $(MAKE) tameclean); \
	done

.PHONY: ready_test
ready_test: all
	sh $(srcdir)/contrib/okws-test-touch-all.sh ; 	\
	for dir in test; do			\
		(cd $$dir && $(MAKE) test); 	\
	done

.PHONY: test 
test:	ready_test
	@echo =======================================================
	@echo "   Starting up OKWS in place for testing!"
	@echo "     - config file: test/system/okws_config"
	@echo "     - Binding to: Port 8081"
	@echo "     - Running okld as: you!"
	@echo "   Hit Control-C to end test."
	@echo ======================================================= 
	okd/okld -f test/system/okws_config	

.PHONY: regtest regtest-stage1
regtest: ready_test
	python $(srcdir)/test/regtest/run.py

regtest-stage1: ready_test
	python $(srcdir)/test/regtest/run.py -m

.PHONY: roottest
roottest: install
	sh contrib/okws-init-jail.sh \
		-f $(okwsconfdir)/okws_config.dist \
		$(okwssvcdir)/
	@echo =======================================================
	@echo "   Starting up OKWS in place for testing!"
	@echo "     - config file: $(okwsconfdir)/okws_config.dist"
	@echo "     - Binding to: Port 80"
	@echo "     - Running okld as: root"
	@echo "   Hit Control-C to end test."
	@echo ======================================================= 
	$(okwsbindir)/okld -f $(okwsconfdir)/okws_config.dist


.PHONY: buildclean
buildclean: tameclean rpcclean
