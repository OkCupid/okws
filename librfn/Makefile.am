
-include $(SFS_LIB_MK)

SUFFIXES = .T .C 
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

okwslib_LTLIBRARIES = librfn.la

librfn_la_SOURCES = \
	math.C \
	rfn3.C \
	primitives.C \
	regex.C \
	filters.C \
	import.C \
	list.C \
	dict.C \
	time.C \
	file.C \
	sync.C

librfn_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = okrfn.h 

EXTRA_DIST =
CLEANFILES = core *.core *~ *.rpo 
MAINTAINERCLEANFILES = Makefile.in
