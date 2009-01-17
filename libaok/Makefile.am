
$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libaok.la

libaok_la_SOURCES = \
	okprot.C \
	ok.C \
	log.C \
	logbuf.C \
	xmlrpcsrv.C \
	locale.C \
	direct.C \
	ok3.C \
	ok3adapter.C

libaok_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = ok.h okprot.h oklog.h svq.h okerr.h xmlrpcsrv.h \
	oklocale.h ok3.h ok3adapter.h

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

ok3adapter.h: $(srcdir)/ok3adapter.Th
	$(TAME) -o $@ $< || (rm -f $@ && false)

ok.o:  ok.C okprot.h
ok.lo: ok.C okprot.h
ok.o: ok.C
ok.lo: ok.C
xmlrpcsrv.C: xmlrpcsrv.h
xmlrpcsrv.o: xmlrpcsrv.C
xmlrpcsrv.lo: xmlrpcsrv.C
okprot.o: okprot.h
okprot.lo: okprot.h
log.o: log.C okprot.h
log.lo: log.C okprot.h
ok3adapter.o: ok3adapter.h
ok3adapter.lo: ok3adapter.h
ok3.lo: okprot.h
ok3.o: okprot.h

xmlrpcsrv.C: xmlrpcsrv.h

.PHONY: rpcclean tameclean

tameclean:
	rm -f ok.C xmlrpcsrv.C xmlrpcsrv.h log.C ok3.C ok3adapter.h

rpcclean:
	rm -f okprot.C okprot.h

dist-hook:
	cd $(distdir) && rm -f okprot.C okprot.h ok.C xmlrpcsrv.h \
		xmlrpcsrv.C ok3.C ok3adapter.h

EXTRA_DIST = .cvsignore ok.T okprot.x xmlrpcsrv.Th xmlrpcsrv.T \
	ok3adapter.Th
CLEANFILES = core *.core *~ *.rpo okprot.C okprot.h ok.C \
	xmlrpcsrv.C xmlrpcsrv.h ok3.C ok3adapter.h

MAINTAINERCLEANFILES = Makefile.in 
