##
## $Id: Makefile.am 2393 2006-12-12 04:23:35Z max $
##
##   test2 is our attempt at regression and Unit tests; eventually, should
##   compile and run out of the box, and should be able to test itself.
##

$(PROGRAMS): $(LDEPS)

if USE_XML

XMLPROGS = xlater

xlater_SOURCES = xlater.C tstprot.C
xlater.o:	xlater.C
xlater.lo:	xlater.C
xlater.o:	tstprot.h

XMLTAMEIN = xlater.T
XMLTAMEOUT = xlater.C

else

XMLPROGS =

endif

TAMEIN = $(XMLTAMEIN) tstsrv.T
TAMEOUT = $(XMLTAMEOUT) tstsrv.C

tstsrv.o: tstprot.h

xdrjson_tst1_SOURCES = xdrjson_tst1.C tstprot.C
okwssvc_PROGRAMS = $(XMLPROGS)
noinst_PROGRAMS = tstsrv xdrjson_tst1
tstsrv_SOURCES = tstsrv.C tstprot.C

SUFFIXES = .x .T
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

tstprot.h: $(srcdir)/tstprot.x
	@rm -f $@
	$(XMLRPCC) -h $(srcdir)/tstprot.x || (rm -f $@ && false)

tstprot.C: $(srcdir)/tstprot.x
	@rm -f $@
	$(XMLRPCC) -c $(srcdir)/tstprot.x || (rm -f $@ && false)

tstprot.C:	tstprot.h

CLEANFILES = core *.core *~ *_config *_log $(TAMEOUNT)
EXTRA_DIST = $(TAMEIN) tstprot.x
MAINTAINERCLEANFILES = Makefile.in

.PHONY:	tameclean

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

