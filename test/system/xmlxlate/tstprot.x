
struct tst_arg_t {
	int x;
	string y<>;
};

struct long_arg_t {
	int x;
	int a<>;
	string b<>;
	string c<>;
	int this_is_a_very_long_name_1;
	int this_is_a_very_long_name_2;
	int this_is_a_very_long_name_3;
	int this_is_a_very_long_name_4;
	int this_is_a_very_long_name_5;
	opaque opq<>;
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
	yy_t a[2];
};

struct aa_t {
	yy_t *p1;
	yy_t *p2;
};

struct bb_t {
	unsigned x;
	hyper y;
	unsigned hyper z;
};

typedef bb_t cc_t<>;

struct rpc_10_arg_t {
       bb_t bb;
       unsigned iters;
};

struct mote_t {
       int questionid;
       unsigned data;
};

typedef mote_t motes_t<>;

struct aston_t {
       unsigned hyper id;
       motes_t questions;
};

struct match_arg_t {
       unsigned hyper h1;
       unsigned hyper h2;
       bool sub;
};


namespace RPC {

program TST_PROG {
	version TEST_VERS {

		void
		TST_NULL(void) = 0;
		
		tst_res_t
		TST_RPC1 (tst_arg_t) = 1;

		ww_t
		TST_RPC2(yy_t) = 2;

		aa_t
		TST_RPC3(aa_t) = 3;

		bb_t
		TST_RPC4(bb_t) = 4;

		bool
		TST_RPC5(cc_t) = 5;

		xx_t
		TST_RPC6(hyper) = 6;

		int
		TST_RPC7(unsigned) = 7;	

		int
		TST_RPC8(long_arg_t) = 8;

		int
		TST_RPC9(aston_t) = 9;
		
		cc_t
		TST_RPC10(rpc_10_arg_t) = 10;

		unsigned hyper 
		TST_RPC11(match_arg_t) = 11;

	} = 1;
} = 34291;

};

%#define XXXX
%#define FOO0  455
%#define FOO1  "xxx"
%#define FOO2  4+5+5 * FOO0 - 400
%#define FOO3  FOO2

