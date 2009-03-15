
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libpub.la

libpub_la_SOURCES = xpub.C txa_prot.C data.C scan.ll parse.yy pubutil.C \
	pub.C xdr.C slave.C rclient.C proxy.C zstr.C okconst.C \
	pvalw.C jail.C lbalance.C rpcwindow.C pub2.C clone.C \
	debug.C stats.C adebug.C scalar.C ranges.C sync.C

libpub_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = pub.h pubutil.h xpub.h txa_prot.h okconst.h \
	fhash.h fhash.h holdtab.h lbalance.h parr.h \
	parse.h pjail.h pub_parse.h puberr.h zstr.h clist.h \
	pslave.h txa.h okdbg.h okdbg-int.h rpcwindow.h pub2.h okclone.h \
	okws_sfs.h timehash.h okstats.h okformat.h ok_adebug.h pscalar.h \
	axprtfd.h oksync.h

noinst_HEADERS =  env.mk

YDEBUG = @YDEBUG@
LDEBUG = @LDEBUG@

SUFFIXES = .T .C .h .x

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)
.x.h:
	-$(RPCC) -h $< || rm -f $@
.x.C:
	-$(RPCC) -c $< || rm -f $@

TAMEIN = pub2.T clone.T adebug.T sync.T
TAMEOUT = pub2.C clone.C adebug.C sync.C
RPCCIN = xpub.x txa_prot.x
RPCCOUT = xpub.C xpub.h txa_prot.C txa_prot.h

pub2.o: pub2.C
pub2.lo: pub2.C
clone.o: clone.C
clone.lo: clone.C
sync.o: sync.C
sync.lo: sync.C
adebug.o: adebug.C
adebug.lo: adebug.C

xpub.h: xpub.x
xpub.C: xpub.x
xpub.o: xpub.h
xpub.lo: xpub.h

txa_prot.h: txa_prot.x
txa_prot.C: txa_prot.x
txa_prot.o: txa_prot.h
txa_prot.lo: txa_prot.h

AM_YFLAGS = -d $(YDEBUG)
AM_LFLAGS = $(LDEBUG)

$(srcdir)/scan.cc: $(srcdir)/scan.ll
	$(LEX) $(AM_LFLAGS) $(LFLAGS) $< && mv $(LEX_OUTPUT_ROOT).c $@
if MAKE_OLD_FLEX 
	@echo "XXX"
	@echo "XXX WARNING!! You have an old version of flex!!"
	@echo "XXX OKWS will leak memory in its pub system!!"
	@echo "XXX Please upgrade!"
	@echo "XXX"
endif

$(srcdir)/parse.cc: $(srcdir)/parse.yy
	$(BISON) $(AM_YFLAGS) $(YFLAGS) -o $@~ $<  \
	  && mv $@~ $@ \
	  && mv $(srcdir)/parse.hh~ $(srcdir)/parse.h

scan.lo: $(srcdir)/scan.cc parse.lo
	if $(LTCXXCOMPILE) $(CXXNOERR) -MT $@ -MD -MP \
	-MF "$(DEPDIR)/$*.Tpo" -c -o $@ $(srcdir)/scan.cc; \
	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Plo"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi

.PHONY: parseclean
parseclean:
	rm -f $(srcdir)/parse.cc $(srcdir)/parse.h $(srcdir)/scan.cc

.PHONY: xdrclean tameclean
xdrclean:
	rm -f $(RPCCOUT)

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(RPCCOUT) $(TAMEOUT)

.PHONY: sourceclean
sourceclean: parseclean xdrclean tameclean

#
# include Makefile flags and options for pysfs1 
#
env.mk: Makefile
	@rm -f env.mk~
	echo "# Makefile fragment generated " `date` > env.mk~
	echo "CC = $(CC)" >> env.mk~
	echo "CXX = $(CXX)" >> env.mk~
	echo "CFLAGS = $(CFLAGS)" >> env.mk~
	echo "CXXFLAGS = $(CXXFLAGS)" >> env.mk~
	echo "LIBS = $(LIBS)" >> env.mk~
	mv -f env.mk~ env.mk


BUILT_SOURCES = parse.cc parse.h scan.cc 
CLEANFILES = core *.core *~ *.rpo $(RPCCOUT) $(TAMEOUT)
EXTRA_DIST = .cvsignore $(RPCCIN) $(TAMEIN)
MAINTAINERCLEANFILES = Makefile.in parse.cc parse.h scan.cc
