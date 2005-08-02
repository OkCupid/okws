/* $Id$ */

%#include "web_prot.h"

%#define TT_ROWS 60
%#define TT_COLS 10


typedef unsigned tt_row_t[TT_COLS];
typedef tt_row_t tt_tab_t[TT_ROWS];

union pt1_times_tab_res_t switch (adb_status_t status) {
 case ADB_OK:
   tt_tab_t tab;
 default:
   void;
};


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
	
	      pt1_times_tab_res_t
	      PT1_TIMES_TAB (void) = 2;
        } = 1;
} = 12210;

%#define PT1_PORT     12210
