
-include $(SFS_LIB_MK)

okwslib_LTLIBRARIES = librfn.la

librfn_la_SOURCES = math.C std_factory.C primitives.C regex.C

librfn_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = okrfn.h

EXTRA_DIST = .cvsignore 
CLEANFILES = core *.core *~ *.rpo 
MAINTAINERCLEANFILES = Makefile.in
