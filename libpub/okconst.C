
#include "okconst.h"
// #include "okwsconf.h"


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
u_int ok_listen_queue_max = 2048;    // arguement to listen (fd, X)
u_int ok_crashes_max = 30;          // number of allowed crashes
u_int ok_csi = 50;                  // crash sampling interval
const char *ok_version = VERSION;
const char *ok_dname = "okd";       // okd name == okd, usually
u_int ok_dport = 80;                // okd port listen to
u_int ok_resurrect_delay = 3;
u_int ok_resurrect_delay_ms = 500;
u_int ok_chld_startup_time = 10;    // startup time in seconds....

//
// helper processes constants
//
u_int hlpr_max_calls = 1000;               // max outstanding calls
u_int hlpr_max_retries = 100;              // ... before giving up
u_int hlpr_retry_delay = 4;                // delay between retries.
u_int hlpr_max_qlen = 1000;                // maximum # to q

//
// path constants -- we should maybe make these relative to a 
// central okd directory
//
const char *ok_jaildir_top = "/var/okws";
const char *ok_topdir = "/usr/local/sbin";
const char *ok_coredumpdir = "/var/coredumps";
const char *ok_sockdir = "/var/run";
const char *ok_jaildir_run = "var/run";
const char *ok_service_bin = "";

//
// log constants for timing
//
u_int ok_log_tick = 500;             // in milliseconds
u_int ok_log_period = 8;             // log flushed every 4 seconds

//
// service/client constants
//
u_int ok_clnt_timeout = 60;          // in seconds
u_int ok_reqsize_limit = 2097152;    // 2MB

//
// libamt
//
u_int ok_amt_lasi = 20;              // load avg sampling interval in secs
u_int ok_ldavg_rpc_timeout = 10;     // load avg RPC timeout in secs
u_int ok_ldavg_ping_interval = 2;    // load avg ping interval in secs
u_int ok_lblnc_launch_timeout = 15;  // wait before timeout in secs

//
// OK Service UID limits
//
int ok_svc_uid_low = 51000;
int ok_svc_uid_high = 52000;

int okwc_def_contlen = 0x20000;      // 128K 
