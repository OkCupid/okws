
#include "okconst.h"
#include "acgiconf.h"


//
// zstr.C constants
//
bool  ok_gzip = true;                        // default gzip
int   ok_gzip_compress_level = 2;            // fast compression
u_int ok_gzip_smallstr = 512;                // combine small strings
u_int ok_gzip_cache_minstr = 0x0;            // smallest to cache
u_int ok_gzip_cache_maxstr = 0x10000;        // largest to cache
u_int ok_gzip_cache_storelimit = 0x1000000;  // 16 M
u_int ok_gzip_mem_level = 9;                 // zlib max


//
// user/group constants
//
const char *ok_uname = "oku";         // runas user for okd, pubd, oklogd
const char *ok_gname = "oku";         // runas grp  for okd, pubd, oklogd
const char *ok_svc_uname = "www";     // unprivileged service user
const char *ok_svc_gname = "www";     // unprivileged service group

//
// port constants
//
u_int ok_mgr_port = 11277;
u_int ok_pubd_port = 11278;

//
// configuration file constants
//
const char *ok_pub_config = "/etc/sfs/pub_config";

//
// timeouts
//
u_int ok_connect_wait = 4;          // seconds
u_int ok_shutdown_timeout = 10;     // seconds
u_int ok_db_retries_max = 100;
u_int ok_db_retries_delay = 3;

//
// okd constants
//
u_int ok_con_queue_max = 40;        // number of req's to queue up
u_int ok_listen_queue_max = 200;    // arguement to listen (fd, X)
u_int ok_crashes_max = 30;          // number of allowed crashes
u_int ok_csi = 50;                  // crash sampling interval
const char *ok_version = VERSION;
const char *ok_dname = "okd";       // okd name == okd, usually
u_int ok_dport = 80;                // okd port listen to
u_int ok_resurrect_delay = 3;
u_int ok_resurrect_delay_ms = 500;

//
// path constants -- we should maybe make these relative to a 
// central okd directory
//
const char *ok_jaildir = "/usr/local/okd/run";
const char *ok_topdir = "/usr/local/okd";

//
// log constants for timing
//
u_int ok_log_tick = 500;             // in milliseconds
u_int ok_log_period = 8;             // log flushed every 4 seconds

//
// service/client constants
//
u_int ok_clnt_timeout = 60;          // in seconds

//
// libamt
//
u_int ok_amt_lasi = 20;              // load avg sampling interval in secs
