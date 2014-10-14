
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libredis.la

libredis_la_SOURCES = redis.C

libredis_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = redis.h 

SUFFIXES = .C .T .h

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

redis.o: redis.C redis.h
redis.lo: redis.C redis.h

.PHONY: rpcclean tameclean

tameclean:
	rm -f redis.C

dist-hook:
	cd $(distdir) && rm -f redis.C

EXTRA_DIST = .cvsignore redis.T
CLEANFILES = core *.core *~ *.rpo redis.C

MAINTAINERCLEANFILES = Makefile.in 
