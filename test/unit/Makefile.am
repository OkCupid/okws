
$(PROGRAMS): $(LDEPS)

SUFFIXES = .C .T .h

.T.C:
	-$(TAME) -c $< || rm -f $@

xmltst1.o:	xmltst1.C
xmltst.lo: 	xmltst1.C

noinst_PROGRAMS = kmptst cgitst1 xmltst1 #pubtst1 pubtst2

# XXX
# these are broken and should be removed
#
#pubtst1_SOURCES = pubtst1.C
#pubtst2_SOURCES = pubtst2.C

cgitst1_SOURCES = cgitst1.C
kmptst_SOURCES = kmptst.C

xmltst1_SOURCES = xmltst1.C

CLEANFILES = core *.core *~
EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in

BUILT_SOURCES = xmltst1.C
