
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libamt.la

libamt_la_SOURCES = mtdispatch.C
libamt_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = amt.h passptr.h

EXTRA_DIST = .cvsignore
CLEANFILES = core *.core *~ *.rpo
MAINTAINERCLEANFILES = Makefile.in 
