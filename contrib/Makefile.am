
okwsutil_SCRIPTS = okws-log-rotate.sh \
		okws-init-jail.sh \
		xmlrpcc-x-collect.pl

okws-init-jail.sh: okws-init-jail.sh.in
	sed 's,%%okwsconfdir%%,$(okwsconfdir), ; ' < $< > $@

EXTRA_DIST = okws-log-rotate.sh \
	okws.sh \
	okws-init-jail.sh.in \
	xmlrpcc-x-collect.pl
