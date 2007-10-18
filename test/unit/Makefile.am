
$(PROGRAMS): $(LDEPS)

SUFFIXES = .C .T .h

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

if USE_XML

XMLPROGS = xmltst1 xmlcli1
xmltst1_SOURCES = xmltst1.C
xmlcli1_SOURCES = xmlcli1.C

xmltst1.o:	xmltst1.C
xmltst1.lo: 	xmltst1.C
xmlcli1.o:	xmlcli1.C
xmlcli1.lo:	xmlcli1.C

else
XMLPROGS =
endif

okwc1tst.o:	okwc1tst.C
okwc1tst.lo:	okwc1tst.C
pubmemtst.o:	pubmemtst.C
pubmemtst.lo: 	pubmemtst.C

TAMEIN = xmlcli1.T xmltst1.T okwc1tst.T pubmemtst.T
TAMEOUT = xmlcli1.C xmltst1.C okwc1tst.C pubmemtst.C

noinst_PROGRAMS = kmptst cgitst1 $(XMLPROGS) xmltst2 okwc1tst pubmemtst \
	gz

cgitst1_SOURCES = cgitst1.C
kmptst_SOURCES = kmptst.C
xmltst2_SOURCES = xmltst2.C
okwc1tst_SOURCES = okwc1tst.C
pubmemtst_SOURCES = pubmemtst.C
gz_SOURCES = gz.C

CLEANFILES = core *.core *~  $(TAMEOUT)
EXTRA_DIST = .cvsignore $(TAMEIN)
MAINTAINERCLEANFILES = Makefile.in

.PHONY: tameclean
tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)
