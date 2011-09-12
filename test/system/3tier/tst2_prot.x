
%#include "web_prot.h"

struct tst2_data_t {
       x_okdate_t d;
       int      i;
       int      pk;
       x_okdate_t d2;
};

typedef tst2_data_t tst2_data_rows_t<>;

struct tst2_put_arg_t {
       string        key<>;
       tst2_data_t  data;
};

struct bar_t {
       tst2_data_t *datum1;
       tst2_data_t *datum2;
       tst2_data_t data<>;
       opaque odata<>;
};

union foo_t switch (bool b) {
case TRUE:
    bar_t bar;
case FALSE:
    void;
};

typedef string tst2_get_arg_t<>;

union tst2_get_res_t switch (adb_status_t status) {
case ADB_OK:
     tst2_data_t dat;
default:
     void;
};

union tst2_mget_res_t switch (adb_status_t status) {
case ADB_OK:
     tst2_data_rows_t rows;
default:
     void;
};

struct tst2_mget_arg_t {
     unsigned sleep_msec;
     unsigned lim;       
};

typedef unsigned hyper u64_vec_t<>;

namespace rpc {

program TST2_PROG {
	version TST2_VERS {

		void
		TST2_NULL(void) = 0;

		adb_status_t
		TST2_PUT(tst2_put_arg_t) = 1;

		tst2_get_res_t
		TST2_GET(tst2_get_arg_t) = 2;

                foo_t
 		TST2_FOO_REFLECT(foo_t) = 3;

		bool
		TST2_NEGATE(bool) = 4;

		unsigned hyper
		TST2_SUM(u64_vec_t) = 5;

		tst2_mget_res_t
		TST2_MGET(tst2_mget_arg_t) = 6;

    	} = 1;
} = 10808;

};

%#define TST2_PORT 10808
