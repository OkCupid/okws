
struct simple_xdr_t {
  unsigned i;
};

program SIMPLE_PROG {
	version SIMPLE_VERS {
		void
		SIMPLE_NULL (void) = 0;

		simple_xdr_t
		SIMPLE_SIMPLE (simple_xdr_t) = 1;
	} = 1;
} = 11280;

%#define SIMPLE_PORT     11280
