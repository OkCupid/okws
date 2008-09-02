
$(PROGRAMS): $(LDEPS) $(LDEPS_PRX)

#-----------------------------------------------------------------------

SUFFIXES = .g .C .T .x .h
.T.C:
	$(TAME) -o $@~ $< && mv $@~ $@
.x.h:	
	$(XMLRPCC) -h -o $@ $< || (rm -f $@ && false)
.x.C:
	$(XMLRPCC) -c -o $@ $< || (rm -f $@ && false)

#-----------------------------------------------------------------------

TAMEIN       = tst2.T
TAMEOUT      = tst2.C

#-----------------------------------------------------------------------

tst2_SOURCES =  tst2_prot.C tst2.C
tst2d_SOURCES = tst2_prot.C tst2d.C

#-----------------------------------------------------------------------

tst2_prot.C:	tst2_prot.h

tst2_prot.o:	tst2_prot.C
tst2_prot.lo:	tst2_prot.C
tst2.o:	      	tst2.C
tst2.lo:	tst2.C
tst2d.o:        tst2_prot.h

#-----------------------------------------------------------------------

okwssvc_PROGRAMS = tst2
okwsprx_PROGRAMS = tst2d

#-----------------------------------------------------------------------

tst2d_LDADD = $(LDADD_AMYSQL)

#-----------------------------------------------------------------------

CLEANFILES = core *.core *~ *_config *_log $(TAMEOUNT)
EXTRA_DIST = $(TAMEIN) tstprot.x
MAINTAINERCLEANFILES = Makefile.in

#-----------------------------------------------------------------------

.PHONY:	tameclean

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

#-----------------------------------------------------------------------
