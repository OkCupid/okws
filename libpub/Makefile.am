
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

okwslib_LTLIBRARIES = libpub.la

libpub_la_SOURCES = \
	pub3prot.C \
	scan.ll \
	parse.yy \
	pubutil.C \
	pub3env.C \
	pub3base.C \
	pub3xdr.C \
	pub3eval.C \
	pub3expr.C \
	pub3expub.C \
	pub3obj.C \
	pub3ast.C \
	pub3json2xdr.C \
	pub3func.C \
	pub3parse.C \
	pub3file.C \
	pub3ref.C \
	pub3cache.C \
	pub3out.C \
	pub3debug.C \
	pub3lib.C \
	pub3eng.C \
	pub3heredoc.C \
	pub3profiler.C \
	precycle.C \
	slave.C \
	zstr.C \
	okconst.C \
	jail.C \
	pub3hilev.C \
	clone.C \
	debug.C \
	stats.C \
	adebug.C \
	scalar.C \
	sync.C \
	escape.C \
	json_rpc.C \
	rxx.C \
	pub3msgpack.C \
	pub3msgpackrpc.C

libpub_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = \
	pub.h \
	pub2.h \
	pubutil.h \
	pub3prot.h \
	pub3base.h \
	okconst.h \
	holdtab.h \
	parse.h \
	pjail.h \
	pub_parse.h \
	zstr.h \
	pslave.h \
	okdbg.h \
	okdbg-int.h \
	okclone.h \
	okws_sfs.h \
	timehash.h \
	okstats.h \
	okformat.h \
	ok_adebug.h \
	pscalar.h \
	oksync.h \
	pub3.h \
	pub3obj.h \
	pub3expr.h \
	pub3func.h \
	pescape.h \
	precycle.h \
	pub3parse.h \
	pub3ast.h \
	pub3file.h \
	pub3eval.h \
	pub3hilev.h \
	pub3cache.h \
	pub3out.h \
	pub3debug.h \
	pub3lib.h \
	pub3eng.h \
	pub3env.h \
	clist.h \
	pub3heredoc.h \
	pub3profiler.h \
	json_rpc.h \
	okws_rxx.h \
	pub3msgpack.h \
	pub3msgpackrpc.h

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

TAMEIN = pub2.T clone.T adebug.T sync.T pub3func.T pub3eval.T
TAMEOUT = pub2.C clone.C adebug.C sync.C pub3func.C pub3eval.C
RPCCIN = pub3prot.x 
RPCCOUT = pub3prot.C pub3prot.h 

pub2.o: pub2.C
pub2.lo: pub2.C
clone.o: clone.C
clone.lo: clone.C
sync.o: sync.C
sync.lo: sync.C
adebug.o: adebug.C
adebug.lo: adebug.C
pub3func.lo: pub3func.C
pub3func.o: pub3func.C

pub3prot.h: pub3prot.x
pub3prot.C: pub3prot.x
pub3prot.o: pub3prot.h
pub3prot.lo: pub3prot.h

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
	echo "OKWS_ABI_VERSION = $(ABI_VERSION)" >> env.mk~
	mv -f env.mk~ env.mk


BUILT_SOURCES = parse.cc parse.h scan.cc 
CLEANFILES = core *.core *~ *.rpo $(RPCCOUT) $(TAMEOUT)
EXTRA_DIST = .cvsignore $(RPCCIN) $(TAMEIN)
MAINTAINERCLEANFILES = Makefile.in parse.cc parse.h scan.cc
