##
## $Id: Makefile.am 2393 2006-12-12 04:23:35Z max $
##
##   test2 is our attempt at regression and Unit tests; eventually, should
##   compile and run out of the box, and should be able to test itself.
##

$(PROGRAMS): $(LDEPS)

if USE_XML

XMLPROGS = xlater

xlater_SOURCES = xlater.C tstprot.C tstprot_xml.C
xlater.o:	xlater.C
xlater.lo:	xlater.C
xlater.o:	tstprot_xml.h tstprot.h

XMLTAMEIN = xlater.T
XMLTAMEOUT = xlater.C

else

XMLPROGS =

endif

TAMEIN = $(XMLTAMEIN) tstsrv.T
TAMEOUT = $(XMLTAMEOUT) tstsrv.C

okwssvc_PROGRAMS = $(XMLPROGS)
noinst_PROGRAMS = tstsrv
tstsrv_SOURCES = tstsrv.C tstprot.C

SUFFIXES = .x .T
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

tstprot.h: $(srcdir)/tstprot.x
	@rm -f $@
	-$(RPCC) -h $(srcdir)/tstprot.x || rm -f $@

tstprot.C: $(srcdir)/tstprot.x
	@rm -f $@
	-$(RPCC) -c $(srcdir)/tstprot.x || rm -f $@

tstprot_xml.h: $(srcdir)/tstprot.x
	@rm -f $@
	../../../xmlrpcc/xmlrpcc -h $(srcdir)/tstprot.x || rm -f $@

tstprot_xml.C : $(srcdir)/tstprot.x 
	@rm -f $@
	../../../xmlrpcc/xmlrpcc -c $(srcdir)/tstprot.x || rm -f $@

tstprot.C:	tstprot.h
tstprot_xml.C:	tstprot_xml.h

CLEANFILES = core *.core *~ *_config *_log $(TAMEOUNT)
EXTRA_DIST = .cvsignore okws_config.in pub_config.in $(TAMEIN)
MAINTAINERCLEANFILES = Makefile.in

.PHONY:	tameclean

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

