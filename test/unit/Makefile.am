
$(PROGRAMS): $(LDEPS)

SUFFIXES = .C .T .h

.T.C:
	-$(TAME) -c $< || rm -f $@

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


noinst_PROGRAMS = kmptst cgitst1 $(XMLPROGS) xmltst2 #pubtst1 pubtst2

cgitst1_SOURCES = cgitst1.C
kmptst_SOURCES = kmptst.C
xmltst2_SOURCES = xmltst2.C

CLEANFILES = core *.core *~
EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in

BUILT_SOURCES = xmltst1.C xmlcli1.C

.PHONY: tameclean
tameclean:
	rm -f $(srcdir)/xmltst1.C $(srcdir)/xmlcli1.C
