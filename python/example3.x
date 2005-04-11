

struct foo_t {
	string x<>;
	unsigned xx;
};

struct bar_t {
	foo_t foos[20];
};

enum baz_typ_t {
	BAZ_NONE = 0,
	BAZ_FOO = 1,
	BAZ_BAR = 2
};

union baz_t switch (baz_typ_t typ)
{
	case BAZ_FOO:
		foo_t foo;
	case BAZ_BAR:
		bar_t bar;
	default:
		void;
};
