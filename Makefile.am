## Process this file with automake to produce Makefile.in
## $Id$


SBD1 = txarpcc libpub pub libahttp libaok libamt libweb  

if USE_MYSQL
SBD2 = libamysql
endif

SUBDIRS = $(SBD1) $(SBD2) okd logd client test etc test2

#test

EXTRA_DIST = setup autoconf.sed .cvsignore 

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
	cmp autoconf.h~ okwsconf.h || mv -f autoconf.h~ okwsconf.h
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
	for dir in libpub libaok okd pub test2 ; do \
		(cd $$dir && $(MAKE) tameclean); \
	done
