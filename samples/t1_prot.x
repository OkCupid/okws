
%#include "amysql_prot.h"
%#include "web_prot.h"

union t1_insert_res_t switch (adb_status_t status)
{
 case ADB_OK:
   unsigned id;
 default:
   void;
};

enum t1_query_typ_t {
  T1_QUERY_ID = 0,
  T1_QUERY_NAME = 1,
  T1_QUERY_EMAIL = 2
};

struct t1_user_t {
  unsigned id;
  string name<>;
  string email<>;
  string zipcode<>;
  unsigned yob;
  sex_t sex;
};

union t1_query_res_t switch (adb_status_t status)
{
 case ADB_OK:
   t1_user_t user;
 default:
   void;
};

union t1_query_arg_t switch (t1_query_typ_t typ)
{
 case T1_QUERY_ID:
   unsigned id;
 case T1_QUERY_NAME:
   string name<>;
 case T1_QUERY_EMAIL:
   string email<>;
};

program T1_PROG {
	version T1_VERS {
		
		void
		T1_NULL (void) = 0;

		t1_insert_res_t	
		T1_INSERT (t1_user_t) = 1;

		t1_query_res_t
		T1_QUERY (t1_query_arg_t) = 2;
	} = 1;
} = 11290;

%#define T1D_PORT        11290
