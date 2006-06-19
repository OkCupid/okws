
okwsutil_DATA = okws-log-rotate.sh okws-init-jail.sh

okws-init-jail.sh: okws-init-jail.sh.in
	sed 's,%%okwsconfdir%%,$(okwsconfdir); ' < $< > $@

EXTRA_DIST = okws-log-rotate.sh okws.sh
