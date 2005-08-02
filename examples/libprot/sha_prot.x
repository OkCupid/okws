/* $Id */

%#include "web_prot.h"

SEED (xf44e29342e19389sdzza341239f8sdf8gqwiiu123098asd45);

typedef string sha_query_arg_t<>;

union sha_query_res_t switch (adb_status_t status)
{
 case ADB_OK:
   string res<>;
 default:
   void;
};

TXA (BAR_TOK) {
program SHA_PROG {
	version SHA_VERS {

		void
		SHA_NULL (void) = 0;

		TXA (FOO_TOK, -BAR_TOK) {
			sha_query_res_t
			SHA_QUERY (sha_query_arg_t) = 1;
		};

		sha_query_res_t
		SHA_QUERY2 (sha_query_arg_t) = 2;	
	} = 1;
} = 11301;
};

%#define SHAD_PORT 4114
