
/* $Id */

enum sex_t {
  NOSEX = 0,
  MALE = 1,
  FEMALE = 2
};

enum adb_status_t {
  ADB_OK = 0,
  ADB_NOT_FOUND = 1,
  ADB_BAD_INSERT = 2,
  ADB_EXISTS = 3,
  ADB_BAD_LOOKUP = 4,
  ADB_BIND_ERROR = 5,
  ADB_EXECUTE_ERROR = 6,
  ADB_ERROR = 7,
  ADB_BAD_VERSION = 8,
  ADB_RPC_ERROR = 9,
  ADB_UNCHANGED = 10,
  ADB_PERM = 11,
  ADB_BAD_QUERY = 12,
  ADB_BAD_FETCH = 13,
  ADB_PARAM_OVERRUN = 14,
  ADB_DEAD_QUERY = 15,
  ADB_BAD_PREPARE = 16,
  ADB_SECURITY_FAILURE = 17
};

struct x_okdate_time_t {
  int sec;
  int min;
  int hour;
};

struct x_okdate_date_t {
  int mday;
  int mon;
  int year;
};

union x_okdate_time_sw_t switch (bool on) {
 case true:
   x_okdate_time_t time;
 default:
   void;
};

union x_okdate_date_sw_t switch (bool on) {
 case true:
   x_okdate_date_t date;
 default:
   void;
};

struct x_okdate_t {
  x_okdate_date_sw_t date;
  x_okdate_time_sw_t time;
};

enum amysql_scalar_type_t {
   AMYSQL_TYPE_ERROR = 0,
   AMYSQL_TYPE_NULL = 1,
   AMYSQL_TYPE_INT = 2,
   AMYSQL_TYPE_DOUBLE = 3,
   AMYSQL_TYPE_STRING = 4,
   AMYSQL_TYPE_DATE = 5,
   AMYSQL_TYPE_BOOL = 6,
   AMYSQL_TYPE_UINT64 = 7,
   AMYSQL_TYPE_OPAQUE = 8
};

typedef string amysql_double_t<>;
typedef string amysql_string_t<>;
typedef opaque amysql_opaque_t<>;

union amysql_scalar_t switch (amysql_scalar_type_t typ) {
case AMYSQL_TYPE_INT:
   int amysql_int;
case AMYSQL_TYPE_DOUBLE:
   amysql_double_t amysql_double;
case AMYSQL_TYPE_STRING:
   amysql_string_t amysql_string;
case AMYSQL_TYPE_DATE:
   x_okdate_t amysql_date;
case AMYSQL_TYPE_OPAQUE:
   amysql_opaque_t amysql_opaque;
case AMYSQL_TYPE_BOOL:
   bool amysql_bool;
case AMYSQL_TYPE_UINT64:
   unsigned hyper amysql_uint64;
default:
   void;
};

typedef amysql_scalar_t amysql_scalars_t<>;

struct amysql_field_t {
  string name<>; 
  string org_name<>;
  string table<>;
  string org_table<>;
  string db<>;
  string catalog<>;
  string def<>;
  unsigned length;
  unsigned max_length;
  int flags;
  int decimals;
  int charsetnr;
  int type;
};

typedef amysql_field_t amysql_fields_t<>;
