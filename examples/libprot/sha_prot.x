
%#include "amysql_prot.h"
%#include "web_prot.h"

typedef string sha_query_arg_t<>;

union sha_query_res_t switch (adb_status_t status)
{
 case ADB_OK:
   string res<>;
 default:
   void;
};

program SHA_PROG {
	version SHA_VERS {

		void
		SHA_NULL (void) = 0;

		sha_query_res_t
		SHA_QUERY (sha_query_arg_t) = 1;
	} = 1;
} = 11301;

%#define SHAD_PORT 4114
