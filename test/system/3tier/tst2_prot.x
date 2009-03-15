
%#include "web_prot.h"

struct tst2_data_t {
       x_okdate_t d;
       int      i;
       int      pk;
       x_okdate_t d2;
};

struct tst2_put_arg_t {
       string        key<>;
       tst2_data_t  data;
};

typedef string tst2_get_arg_t<>;

union tst2_get_res_t switch (adb_status_t status) {
case ADB_OK:
     tst2_data_t dat;
default:
     void;
};


program TST2_PROG {
	version TST2_VERS {

		void
		TST2_NULL(void) = 0;

		adb_status_t
		TST2_PUT(tst2_put_arg_t) = 1;

		tst2_get_res_t
		TST2_GET(tst2_get_arg_t) = 2;

    	} = 1;
} = 10808;

%#define TST2_PORT 10808
