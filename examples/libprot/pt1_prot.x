%#include "amysql_prot.h"
%#include "web_prot.h"

struct pt1_out_t {
   int id;
   string sha1<>;
};

union pt1_query_res_t switch(adb_status_t status)
{
  case ADB_OK :
     pt1_out_t out;
  default:
     void;
};

program PT1_PROG {
        version PT1_VERS {
              void
              PT1_NULL (void) = 0;

              pt1_query_res_t 
              PT1_QUERY (int) = 1;
        } = 1;
} = 12210;

%#define PT1_PORT     12210
