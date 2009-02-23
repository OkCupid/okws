
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS)

SUFFIXES = .C .T .h
.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)

TAMEIN = okssld.T
TAMEOUT = okssld.C

if USE_SSL
okwsexec_PROGRAMS = okssld
okssld_SOURCES = okssld.C
okssld.o: okssld.C
okssld.lo: okssld.C
endif


.PHONY: tameclean

tameclean:
	rm -f $(TAMEOUT)

dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT)

EXTRA_DIST = $(TAMEIN)
CLEANFILES = core *.core *~ *.rpo $(TAMEOUT)
MAINTAINERCLEANFILES = Makefile.in 
