
-include $(SFS_LIB_MK)

#-----------------------------------------------------------------------

ezdb_prot.C: ezdb_prot.h
ezdb_prot.o: ezdb_prot.C
ezdb_prot.lo: ezdb_prot.C
ezdb_ag.C: ezdb_ag.h
ezdb_ag.o: ezdb_ag.C
ezdb_ag.lo: ezdb_ag.C

#-----------------------------------------------------------------------

SUFFIXES = .C .T .h .x

.x.h:
	$(OKRPCC) -h $< || (rm -f $@ && false)
.x.C:
	$(OKRPCC) -c $< || (rm -f $@ && false)

RPCCIN = ezdb_prot.x
RPCCOUT = ezdb_prot.h ezdb_prot.C

#-----------------------------------------------------------------------

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

TAMEIN = ezdb.T
TAMEOUT = ezdb.C

#-----------------------------------------------------------------------

ezdb_ag.h: $(srcdir)/ezdb_ag.pl
	@rm -f $@
	-$(PERL) $< -h > $@~ && mv $@~ $@
ezdb_ag.C: $(srcdir)/ezdb_ag.pl
	@rm -f $@
	-$(PERL) $< -c > $@~ && mv $@~ $@

PERLOUT = ezdb_ag.C ezdb_ag.h
PERLIN = ezdb_ag.pl

#-----------------------------------------------------------------------

okwslib_LTLIBRARIES = libezdb.la libezdbsrv.la 
okwsinclude_HEADERS = ezdb_prot.h ezdb.h ezdb_field.h ezdb_ag.h ezdb_serve.h \
	ezdb_bind.h

#-----------------------------------------------------------------------

libezdb_la_SOURCES = ezdb_prot.C field.C ezdb_ag.C ezdb.C  bind.C
libezdb_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

#-----------------------------------------------------------------------

libezdbsrv_la_SOURCES = serve.C
libezdbsrv_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

#-----------------------------------------------------------------------

CLEANFILES = core *.core *~ *_config *_log $(TAMEOUNT)
EXTRA_DIST = $(TAMEIN) ezdb_prot.x
MAINTAINERCLEANFILES = Makefile.in

#-----------------------------------------------------------------------

.PHONY:	tameclean

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

#-----------------------------------------------------------------------
