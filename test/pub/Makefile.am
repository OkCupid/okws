PACKAGE_STRING=pub
TESTS = \
	scoping_for.pub \
	scoping_globals-locals.pub \
	scoping_order_of_decl.pub \
	scoping_function.pub \
	scoping_if-switch-capture.pub \
	scoping_include.pub \
	scoping_uniref.pub \
	undef_vs_null.pub

AM_TESTS_ENVIRONMENT=PUB=$(top_builddir)/pub/pub3; export PUB;
LOG_COMPILER = $(SH) $(srcdir)/test_runner

.phony: test

test: check
