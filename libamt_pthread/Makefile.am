
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libamt_pthread.la

libamt_pthread_la_SOURCES = mtdispatch_pthread.C
libamt_pthread_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = amt_pthread.h

EXTRA_DIST =
CLEANFILES = core *.core *~ *.rpo
MAINTAINERCLEANFILES = Makefile.in 
