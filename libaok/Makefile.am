
$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libaok.la

libaok_la_SOURCES = okprot.C ok.C log.C logbuf.C xmlrpcsrv.C

okwsinclude_HEADERS = ok.h okprot.h oklog.h svq.h okerr.h xmlrpcsrv.h

noinst_HEADERS = okprot.x

okprot.h: $(srcdir)/okprot.x
	@rm -f $@
	-$(RPCC) -h $(srcdir)/okprot.x || rm -f $@

okprot.C : $(srcdir)/okprot.x okprot.h
	@rm -f $@
	-$(RPCC) -c $(srcdir)/okprot.x || rm -f $@

SUFFIXES = .C .T .h

.T.C:
	-$(TAME) -c $< || rm -f $@

xmlrpcsrv.h: $(srcdir)/xmlrpcsrv.Th
	-$(TAME) -o $@ $< || rm -f $@

ok.o:  ok.C
ok.lo: ok.C
xmlrpcsrv.o: xmlrpcsrv.C
xmlrpcsrv.lo: xmlrpcsrv.C

xmlrpcsrv.C: xmlrpcsrv.h

.PHONY: rpcclean tameclean

tameclean:
	rm -f $(srcdir)/ok.C $(srcdir)/xmlrpcsrv.C \
		$(srcdir)/xmlrpcsrv.h

rpcclean:
	rm -f $(srcdir)/okprot.C $(srcdir)/okprot.h



EXTRA_DIST = .cvsignore ok.T xmlrpcsrv.Th xmlrpcsrv.C
CLEANFILES = core *.core *~ *.rpo

MAINTAINERCLEANFILES = Makefile.in config.guess config.h.in config.sub \
	configure install-sh ltconfig ltmain.sh missing mkinstalldirs \
	stamp-h.in 

BUILT_SOURCES = okprot.C okprot.h ok.C xmlrpcsrv.h
