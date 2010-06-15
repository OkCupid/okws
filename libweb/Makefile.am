
-include $(SFS_LIB_MK)

$(PROGRAMS): $(LDEPS) 

okwslib_LTLIBRARIES = libweb.la

libweb_la_SOURCES = web_prot.C inputcheck.C web.C email.C okwc.C \
	 okwcxml.C okwc3.C okwc4.C

libweb_la_LDFLAGS = $(LIBTOOL_VERSION_INFO)

okwsinclude_HEADERS = web_prot.h web.h email.h okwc.h  \
	okwcxml.h okwc3.h okwc4.h 

SUFFIXES = .C .T .h .x

.T.C:
	$(TAME) -o $@ $< || (rm -f $@ && false)
.x.h:
	$(OKRPCC) -h $< || (rm -f $@ && false)
.x.C:
	$(OKRPCC) -c $< || (rm -f $@ && false)

TAMEIN = okwcxml.T okwc3.T okwc4.T
RPCCIN = web_prot.x
TAMEOUT = okwcxml.C okwc3.C okwc4.C
RPCCOUT = web_prot.h web_prot.C

okwc3.o:	okwc3.C
okwc3.lo:	okwc3.C
okwc4.o:	okwc4.C
okwc4.lo:	okwc4.C
okwcxml.o:	okwcxml.C
okwcxml.lo:	okwcxml.C

web_prot.h:	web_prot.x
web_prot.C:	web_prot.x
web_prot.o:	web_prot.h
web_prot.lo: 	web_prot.h

.PHONY: xdrclean tameclean sourceclean

xdrclean:
	rm -f $(RPCCOUT)

tameclean:
	rm -f $(TAMEOUT)

soureclean: xdrclean tameclean

EXTRA_DIST = .cvsignore $(TAMEIN) $(RPCCIN)
CLEANFILES = core *.core *~ *.rpo $(TAMEOUT) $(RPCCOUT)
MAINTAINERCLEANFILES = Makefile.in


dist-hook:
	cd $(distdir) && rm -f $(TAMEOUT) $(RPCCOUT)
