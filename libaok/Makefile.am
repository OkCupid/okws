
$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libaok.la

libaok_la_SOURCES = \
	okprot.C \
	ok.C \
	log.C \
	logbuf.C \
	xmlrpcsrv.C \
	locale.C \
	direct.C

libaok_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = ok.h okprot.h oklog.h svq.h okerr.h xmlrpcsrv.h \
	oklocale.h

okprot.h: $(srcdir)/okprot.x
	@rm -f $@
	-$(RPCC) -h $(srcdir)/okprot.x || rm -f $@

okprot.C : $(srcdir)/okprot.x okprot.h
	@rm -f $@
	-$(RPCC) -c $(srcdir)/okprot.x || rm -f $@

SUFFIXES = .C .T .h

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

xmlrpcsrv.h: $(srcdir)/xmlrpcsrv.Th
	$(TAME) -o $@ $< || (rm -f $@ && false)

ok.o:  ok.C okprot.h
ok.lo: ok.C okprot.h
xmlrpcsrv.C: xmlrpcsrv.h
xmlrpcsrv.o: xmlrpcsrv.C
xmlrpcsrv.lo: xmlrpcsrv.C
okprot.o: okprot.h
okprot.lo: okprot.h
log.o: log.C
log.lo: log.C

xmlrpcsrv.C: xmlrpcsrv.h

.PHONY: rpcclean tameclean

tameclean:
	rm -f ok.C xmlrpcsrv.C xmlrpcsrv.h log.C

rpcclean:
	rm -f okprot.C okprot.h

dist-hook:
	cd $(distdir) && rm -f okprot.C okprot.h ok.C xmlrpcsrv.h \
		xmlrpcsrv.C

EXTRA_DIST = .cvsignore ok.T okprot.x xmlrpcsrv.Th xmlrpcsrv.T
CLEANFILES = core *.core *~ *.rpo okprot.C okprot.h ok.C \
	xmlrpcsrv.C xmlrpcsrv.h

MAINTAINERCLEANFILES = Makefile.in 
