PACKAGE_STRING=pub
TESTS = \
	dict_equal.pub \
	scoping_for.pub \
	scoping_globals-locals.pub \
	scoping_order_of_decl.pub \
	scoping_function.pub \
	scoping_if-switch-capture.pub \
	scoping_include.pub \
	scoping_uniref.pub \
	scoping_specifiers.pub \
	syntax_error.pub \
	undef_vs_null.pub

if SFS_DEBUG
    TESTS+=blocking_call_warn.pub
endif
AUTOMAKE_OPTIONS = parallel-tests
LOG_COMPILER = $(builddir)/test_runner
