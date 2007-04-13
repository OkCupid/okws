

$(PROGRAMS): $(LDEPS)

SUFFIXES = .C .T

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

okwslib_LTLIBRARIES = libokxml.la

libokxml_la_SOURCES = xmlparse.C xmldata.C xmlobj.C xmlxlate.C \
	xmlxlatemgr.C

okwsinclude_HEADERS = okxml.h okxmldata.h okxmlobj.h okxmlparse.h smartvec.h\
	okxmlxlate.h okxmlxlatemgr.h

TAMEOUT = xmlxlatemgr.C

xmlxlatemgr.o:	xmlxlatemgr.C
xmlxlatemgr.lo:	xmlxlatemgr.C

EXTRA_DIST = 
CLEANFILES = core *.core *~ *.rpo $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

MAINTAINERCLEANFILES = Makefile.in 
