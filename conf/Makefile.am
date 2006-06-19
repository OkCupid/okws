
EXTRA_DIST = okws_config.dist.in pub_config.dist.in

okwsconf_DATA = okws_config.dist pub_config.dist

okws_config.dist: okws_config.dist.in
	sed "s,%%okwslibdir%%,$(okwslibdir), ; s,%%okwsconfdir%%,$(okwsconfdir), ; s,%%okws_prefix%%,$(okws_prefix), ;" < $< > $@

pub_config.dist: pub_config.dist.in
	sed "s,%%okwshtdocsdir%%,$(okwshtdocsdir), ; " < $< > $@

SUBDIRS =  freebsd
