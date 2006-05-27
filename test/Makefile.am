
$(PROGRAMS): $(LDEPS)

noinst_PROGRAMS = kmptst cgitst1 #pubtst1 pubtst2

# XXX
# these are broken and should be removed
#
#pubtst1_SOURCES = pubtst1.C
#pubtst2_SOURCES = pubtst2.C

cgitst1_SOURCES = cgitst1.C
kmptst_SOURCES = kmptst.C

CLEANFILES = core *.core *~
EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = Makefile.in
