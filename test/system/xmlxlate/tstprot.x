

struct tst_arg_t {
	int x;
	string y<>;
};

struct tst_res_t {
	bool b;
	int v<>;
};


program TST_PROG {
	version TEST_VERS {

		void
		TST_NULL(void) = 0;
		
		tst_res_t
		TST_RPC1 (tst_arg_t) = 1;

	} = 1;
} = 34291;
