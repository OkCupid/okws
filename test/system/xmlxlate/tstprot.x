

struct tst_arg_t {
	int x;
	string y<>;
};

struct tst_res_t {
	bool b;
	int v<>;
};

enum xx_t {
	XXA = 1,
	XXB = 2,
	XXC = 3
};

union yy_t switch (xx_t xx)
{
case XXA:
	tst_arg_t a;
case XXB:
	int x;
default:
	int z;	
};

typedef string zz_t<>;

struct ww_t {
	zz_t z;
	yy_t v<>;
	yy_t a[50];
};


program TST_PROG {
	version TEST_VERS {

		void
		TST_NULL(void) = 0;
		
		tst_res_t
		TST_RPC1 (tst_arg_t) = 1;

		ww_t
		TST_RPC2(yy_t) = 2;

	} = 1;
} = 34291;
