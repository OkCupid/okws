

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libokxml.la

libokxml_la_SOURCES = xmlparse.C xmldata.C xmlwrap.C

okwsinclude_HEADERS = okxml.h okxmldata.h okxmlwrap.h okxmlparse.h

EXTRA_DIST = .cvsignore
CLEANFILES = core *.core *~ *.rpo

MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 
