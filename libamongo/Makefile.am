-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

SUFFIXES = .T .C
.Th.h:
	$(TAME) -o $@ $< || (rm -f $@ && false)

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)


okwslib_LTLIBRARIES = libamongo.la
libamongo_la_SOURCES = abson.C amongo.C
okwsinclude_HEADERS = abson.h amongo.h

libamongo_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

CLEANFILES = amongo.C amongo.h
EXTRA_DIST = amongo.T amongo.Th
BUILT_SOURCES = amongo.h
amongo.$(objext): amongo.h

MAINTAINERCLEANFILES = Makefile.in
