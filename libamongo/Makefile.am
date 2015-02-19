-include $(SFS_LIB_MK)

OKMONGO_SRC=$(srcdir)/okmongo/src
VPATH=$(srcdir):$(OKMONGO_SRC)

$(PROGRAMS): $(LDEPS)

SUFFIXES = .T .C
.Th.h:
	$(TAME) -o $@ $< || (rm -f $@ && false)

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)


okwslib_LTLIBRARIES = libamongo.la
libamongo_la_SOURCES =\
	abson.C\
	amongo.C\
	bson.cc\
	bson_dumper.cc\
	mongo.cc

okwsinclude_HEADERS = \
	abson.h\
	amongo.h\
	$(OKMONGO_SRC)/bson.h\
	$(OKMONGO_SRC)/mongo.h\
	$(OKMONGO_SRC)/string_matcher.h\
	$(OKMONGO_SRC)/bson_dumper.h

libamongo_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

CLEANFILES = amongo.C amongo.h
EXTRA_DIST = amongo.T amongo.Th
BUILT_SOURCES = amongo.h
amongo.$(objext): amongo.h

MAINTAINERCLEANFILES = Makefile.in
