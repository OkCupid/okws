
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
  ADB_ERROR = 7
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
