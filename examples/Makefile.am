## Process this file with automake to produce Makefile.in
## $Id$

SUBDIRS = libprot websrv dbprox

EXTRA_DIST = setup autoconf.sed .cvsignore

CLEANFILES = core *.core *~

MAINTAINERCLEANFILES = 					\
	aclocal.m4 install-sh mkinstalldirs missing depcomp \
	configure config.guess config.sub config.h.in	\
	INSTALL COPYING Makefile.in stamp-h.in

.PHONY: jail
jail:
	for dir in websrv; do \
		(cd $$dir && $(MAKE) jail); \
	done

if DLINKED_SERVICES
install-data-hook: jail
else
install-data-hook:
endif
if DLINKED_SERVICES
endif
	@echo
	@echo "********************** Jailed Install **********************"
	@echo OKWS Jail Directory:
	@echo
	@echo JailDir $(okmjaildir)
if DLINKED_SERVICES
	@echo
	@echo Jailed Directory outfitted with shared objects!
endif
	@echo "************************************************************"
