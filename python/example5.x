

struct foo_t {
	string x<>;
	unsigned xx;
};

program FOO_PROG {
	version FOO_VERS {

		void
		FOO_NULL(void) = 0;
	
		unsigned
		FOO_FUNC (foo_t) = 1;
	} = 1;
} = 100;
