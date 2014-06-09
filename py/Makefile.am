# We install the python library via this makefile instead of using distutils
# (to respect $prefix, $dstdir work with make uninstall...)

BUILT_SOURCE=okws/_config.py okws/_pubast.py
CLEANFILES=okws/_config.py okws/_pubast.py

pythonlibdir=$(pkglibdir)/python/$(PACKAGE)

ASTGEN=$(top_srcdir)/tools/ast_gen.py

okws/_config.py: gen_okws_config.sh
	-$(SHELL) $< > $@ || rm -f $@

okws/_pubast.py: $(ASTGEN) $(top_srcdir)/libpub/pub3prot.x
	-$(PYTHON) $(ASTGEN) $(top_srcdir)/libpub/pub3prot.x "xpub3_file_t" > $@  \
	 || rm -f $@

pythonlib_DATA= \
	okws/_config.py \
	okws/__init__.py \
	okws/pubast.py \
	okws/_pubast.py
