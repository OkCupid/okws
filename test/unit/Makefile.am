
$(PROGRAMS): $(LDEPS)

SUFFIXES = .C .T .h

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

if USE_XML

XMLPROGS = xmltst1 xmlcli1 xmltst3 xmltst4 xmltst5 xmltst6
xmltst1_SOURCES = xmltst1.C
xmltst3_SOURCES = xmltst3.C
xmltst4_SOURCES = xmltst4.C
xmltst5_SOURCES = xmltst5.C
xmltst6_SOURCES = xmltst6.C
xmlcli1_SOURCES = xmlcli1.C

xmltst1.o:	xmltst1.C
xmltst1.lo: 	xmltst1.C
xmlcli1.o:	xmlcli1.C
xmlcli1.lo:	xmlcli1.C
xmltst3.o:	xmltst3.C
xmltst3.lo:	xmltst3.C
xmltst4.o:	xmltst4.C
xmltst4.lo:	xmltst4.C
xmltst5.lo:	xmltst5.C	
xmltst5.o:	xmltst5.C	
xmltst6.o:	xmltst6.C
xmltst6.lo:	xmltst6.C
srv.o:		srv.C
srv.lo:		srv.T

else
XMLPROGS =
endif

okwc1tst.o:	    okwc1tst.C
okwc1tst.lo:	okwc1tst.C
pubmemtst.o:	pubmemtst.C
pubmemtst.lo: 	pubmemtst.C

TAMEIN = \
	xmlcli1.T \
	pubmemtst.T \
	okwc1tst.T \
	xmltst1.T \
	xmltst3.T \
	xmltst4.T \
	xmltst5.T \
	xmltst6.T \
	srv.T 

TAMEOUT = \
	xmlcli1.C \
	pubmemtst.C \
	okwc1tst.C \
	xmltst1.C \
	xmltst3.C \
	xmltst4.C \
	xmltst5.C \
	xmltst6.C \
	srv.C 

noinst_PROGRAMS = \
	kmptst \
	cgitst1 \
	$(XMLPROGS) \
	xmltst2 \
	okwc1tst \
	pubmemtst \
	gz \
	srv \
	newline

cgitst1_SOURCES = cgitst1.C
kmptst_SOURCES = kmptst.C
xmltst2_SOURCES = xmltst2.C
okwc1tst_SOURCES = okwc1tst.C
pubmemtst_SOURCES = pubmemtst.C
gz_SOURCES = gz.C
srv_SOURCES = srv.C
newline_SOURCES = newline.C

CLEANFILES = core *.core *~  $(TAMEOUT)
EXTRA_DIST = .cvsignore $(TAMEIN)
MAINTAINERCLEANFILES = Makefile.in

.PHONY: tameclean
tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)
