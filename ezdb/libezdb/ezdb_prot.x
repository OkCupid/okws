%#include "web_prot.h"

struct ezdb_sth_t {
   unsigned hyper id; 
};

union ezdb_prepare_res_t switch (adb_status_t status) {
case ADB_OK:
    ezdb_sth_t sth;
default:
    void;
};

struct ezdb_code_location_t {
    string file<>;
    int line;
};

struct ezdb_execute_arg_t {
    ezdb_code_location_t code_location;
    bool safe;
    string query<>;
    amysql_scalars_t args;
};

struct ezdb_error_pair_t {
   int code;
   string desc<>;
};

typedef string ezdb_fieldname_t<>;

typedef unsigned hyper sth_id_t;

struct ezdb_execute_ok_t {
    sth_id_t sth_id;
    unsigned num_rows;
    amysql_field_t fields<>;
    unsigned hyper insert_id;
};

union ezdb_execute_res_t switch (adb_status_t status) {
case ADB_OK:
   ezdb_execute_ok_t res;
default:
   ezdb_error_pair_t error;
};

struct ezdb_fetch_arg_t 
{
   sth_id_t sth_id;
   unsigned int row;
};

union ezdb_fetch_res_t switch (adb_status_t status) {
case ADB_OK:
   amysql_scalars_t row;
default:
   ezdb_error_pair_t error;
};

namespace rpc {

program EZDB_PROG {
	version EZDB_VERS {

		void
		EZDB_NULL(void) = 0;

		ezdb_execute_res_t
		EZDB_EXECUTE(ezdb_execute_arg_t) = 1;

		ezdb_fetch_res_t
		EZDB_FETCH(ezdb_fetch_arg_t) = 2;
		
		adb_status_t
		EZDB_FINISH(sth_id_t) = 3;

    	} = 1;
} = 38777;

};

%#define EZDBD_PORT 38777
