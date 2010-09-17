
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

SUFFIXES = .C .T

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

okwslib_LTLIBRARIES = libokxml.la

libokxml_la_SOURCES = xmlparse.C xmldata.C xmlobj.C xmlxlate.C \
	xmlxlatemgr.C xmlgeneric.C xml_ahparse.C xdr_as_json.C

libokxml_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = okxml.h okxmldata.h okxmlobj.h okxmlparse.h smartvec.h\
	okxmlxlate.h okxmlxlatemgr.h okxmlgeneric.h xml_ahparse.h \
	xdr_as_json.h

TAMEOUT = xmlxlatemgr.C

xmlxlatemgr.o:	xmlxlatemgr.C
xmlxlatemgr.lo:	xmlxlatemgr.C

EXTRA_DIST = xmlxlatemgr.T
CLEANFILES = core *.core *~ *.rpo $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

MAINTAINERCLEANFILES = Makefile.in 
