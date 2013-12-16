/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include <limits.h>
#include "okconst.h"
#include "sfsmisc.h"
#include "rxx.h"


//
// zstr.C constants
//
gzip_mode_t ok_gzip_mode = GZIP_SMART;       // default gzip
int   ok_gzip_compress_level = 2;            // fast compression
u_int ok_gzip_smallstr = 512;                // combine small strings
u_int ok_gzip_cache_minstr = 0x0;            // smallest to cache
u_int ok_gzip_cache_maxstr = 0x10000;        // largest to cache
u_int ok_gzip_cache_storelimit = 0x1000000;  // 16 M
u_int ok_gzip_mem_level = 9;                 // zlib max
int   ok_gzip_naive_compress_level = 7;      // naive gzip compress level


//
// chunking tuning
//
bool ok_gzip_chunking = true;              // prefer chunked w/ gzip
bool ok_gzip_chunking_old_safaris = false; // some safaris are broken
bool ok_gzip_error_pages = false;          // save space on 404s?

//
// user/group constants
//
const char *ok_okd_uname = "oku";     // unprivileged service user
const char *ok_okd_gname = "oku";     // unprivileged service group

const char *ok_pubd_uname = "www";    // default runas user for pubd
const char *ok_pubd_gname = "www";    // default runas group for pubd

const char *ok_logd_uname = "oklog";  // logging role, user
const char *ok_logd_gname = "oklog";  // logging role, group

const char *ok_ssl_uname = "okssl";  // SSL key management role, user
const char *ok_ssl_gname = "okssl";  // SSL key management role, group

//
// pub constants
//
const char *ok_pubobjname = "pub";

//
// port constants
//
u_int ok_mgr_port = 11277;
u_int ok_pubd_port = 11278;

//
// configuration file constants
//
const char *ok_pub_config = "pub_config";
const char *ok_etc_dir0 = OKWS_CONFIG_DIR;
const char *ok_etc_dir1 = "/usr/local/etc/okws";
const char *ok_etc_dir2 = "/usr/local/okws/conf";
const char *ok_etc_dir3 = "/etc/okws";
const char *ok_okws_config = "okws_config";
const char *ok_okd_config = "okd_config";
const char *okd_mgr_socket = "/var/run/okd.sock";

int okd_mgr_socket_mode = 0600;

//
// Whether okd turns on TCP_NODELAY on incoming connections.  It used
// to do so by default, but now we're not turning on TCP_NODELAY
// by default, meaning, Nagle algorithm still in operation.
//
bool okd_tcp_nodelay = false;

//
// FDFD default command line arg
//
const char *ok_fdfd_command_line_flag = "-s";

//
// default packet size for all axprts
//
size_t ok_axprt_ps = 0x1000000;   // 16MB -- big for now

//
// http constants
//
const char *ok_http_urlencoded = "application/x-www-form-urlencoded";
const char *ok_http_multipart  = "multipart/form-data";

//
// timeouts
//
u_int ok_connect_wait = 4;          // seconds
u_int ok_shutdown_timeout = 10;     // seconds
u_int ok_shutdown_timeout_fast = 3; // seconds
u_int ok_shutdown_retries = 3;      // n retries before giving up
u_int ok_db_retries_max = 100;
u_int ok_db_retries_delay = 3;
u_int ok_demux_timeout = 30;        // clients have 30 secs to make a REQ
u_int ok_ka_timeout = 10;           // clients have 10 secs to use a ka conn

//
// okd constants
//
u_int ok_con_queue_max = 40;        // number of req's to queue up
u_int ok_listen_queue_max = 2048;   // arguement to listen (fd, X)
u_int ok_crashes_max = 30;          // number of allowed crashes
u_int ok_csi = 50;                  // crash sampling interval
const char *ok_version = VERSION;
const char *ok_dname = "okd";       // okd name == okd, usually
const char *ok_wsname = "okws";     // okws name == okws, usually
u_int ok_dport = 80;                // okd port listen to
u_int ok_resurrect_delay = 3;
u_int ok_resurrect_delay_ms = 500;
u_int ok_chld_startup_time = 10;    // startup time in seconds....
u_int ok_filter_cgi = XSSFILT_ALL;  // filter XSS metacharacters
u_int okd_fds_high_wat = 500;       // turn off accept above this
u_int okd_fds_low_wat = 450;        // below low wat, turn accept back on.
u_int ok_svc_fds_high_wat = 1000;   // a little bit less thatn 1024 for now.
u_int ok_svc_fds_low_wat = 950;     // below low wat, turn accept back on.
u_int ok_svc_fd_quota = 100;        // quota for SVC fds.
bool okd_accept_msgs = true;        // display messages about accept()
bool ok_svc_accept_msgs = true;     // display messages about accept()
u_int ok_ssdi = 0;                  // syscall stat dump interval
bool ok_send_sin = true;            // send sin's over FD chan?
bool okd_child_sel_disable = false; // disable read sel on child
u_int okd_debug_msg_freq = 0;       // disable okd debug messages
int64_t okws_debug_flags = 0;       // no debug on at first
bool ok_dangerous_zbufs = false;    // only for experts if true.
int ok_svc_life_reqs = 0;           // Upper limit on # of Reqs per Lifetime
int ok_svc_life_time = 0;           // Upper limit on service lifetime
u_int okd_accept_delay = 0;         // delay before enabling accept

time_t okd_emergency_kill_wait_time = 5;    // 5s of inactivity -> kill
int okd_emergency_kill_signal = SIGABRT;    // signal to send for kill
time_t okd_sendcon_time_budget = 10;        // >10s, something is F'ed

//
// okld constants
//
u_int okld_startup_batch_size = 25;        // number in each batch
u_int okld_startup_batch_wait = 3;         // n secs to wait between batch

//
// okssl constants
//
const char *ok_ssl_certfile = "okws.crt";     // name of the OKWS certfile
const char *ok_ssl_keyfile = "okws.key";      // private Key
u_int ok_ssl_timeout = 100;                   // long timeout for SSL con
u_int ok_ssl_port = 443;                      // default SSL port

// location of the memory-mapped clock daemon
const char *ok_mmcd = "/usr/local/lib/sfslite/mmcd";

// location of mmcd's memory mapped file
const char *ok_mmc_file = "/var/run/mmcd.mmf";

// what OKWS reports it is in HTTP responses
const char *okws_server_label = "OKWS/" VERSION;


//
// helper processes constants
//
u_int hlpr_max_calls = 1000;               // max outstanding calls
u_int hlpr_max_retries = UINT_MAX;         // ... before giving up
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
const char *ok_logd_pidfile = "oklogd.pid";
const char *ok_coredump_user = ok_root;
const char *ok_coredump_group = ok_wheel;
int ok_coredump_mode = 0400;
size_t ok_max_brother_procs = 20;

//
// log constants for timing
//
time_t ok_log_tick = 250;            // in milliseconds
size_t ok_log_period = 4;            // log flushed every 4 ticks
size_t ok_log_hiwat = 256;           // max # of entries to buffer

str   ok_syslog_priority = "local3.debug";
str   ok_syslog_tag = "okws";
str   ok_syslog_domain = "local3";
str   ok_access_log_fmt = "ivt1sbz";

//
// service/client constants
//
u_int ok_clnt_timeout = 60;          // in seconds

size_t ok_hdrsize_limit = 0x2000;      // 8K
size_t ok_reqsize_limit = 0x200000;    // 2MB
size_t ok_cgibuf_limit = 0x80000;      // 512KB

//
// Specify which select policy to use; by default, SELECT_NONE
// is don't use any select policy.
//
sfs_core::select_policy_t ok_sys_sel_policy = sfs_core::SELECT_NONE; 

//
// libamt
//
u_int ok_amt_lasi = 20;              // load avg sampling interval in secs
u_int ok_ldavg_rpc_timeout = 10;     // load avg RPC timeout in secs
u_int ok_ldavg_ping_interval = 2;    // load avg ping interval in secs
u_int ok_lblnc_launch_timeout = 15;  // wait before timeout in secs
u_int ok_amt_report_q_freq = 0;      // disable reporting by default
u_int ok_amt_q_timeout = 60;         // timeout RPCs longer than 1 minute
u_int ok_amt_stat_freq = 60;         // statistics frequency
bool ok_amt_rpc_stats = true;        // whether to turn on RPC stats
u_int ok_amt_rpc_stats_interval = 300;   // interval to printf (5min)
bool ok_kthread_safe = false;        // by default assume PTH / no kthreads

//
// ahttpcon debug, etc
//
bool ok_ahttpcon_zombie_warn = false;
time_t ok_ahttpcon_zombie_timeout = 120; // 2m timeout for ahttpcons

//
// OK Service UID limits
//
int ok_svc_uid_low = 51000;
int ok_svc_uid_high = 52000;
int ok_svc_mode = 0410;
int ok_interpreter_mode = 0550;
int ok_script_mode = 0440;

int okwc_def_contlen = 0x20000;      // 128K 
size_t okwc_scratch_sz = 0x1000;

//
// for freebsd this works, but no on planetlab, etc..
//
const char *ok_root = "root";
const char *ok_wheel = "wheel";

//
// pub2 constants
//
int ok_pub3_wss = 0;
int ok_pub3_refresh_max = 60;    // maximum time to wait between refreshes
int ok_pub3_refresh_min = 5;     // minimum time to wait
int ok_pub3_refresh_incr = 2;    // value to increment by if inactivity
int ok_pub3_caching = 1;         // turn caching on by default
int ok_pub3_viserr = 1;          // pub errors visible by default

// check treestat sentinel every two seconds
int ok_pub3_treestat_interval = 2;   

// In evaluating lambdas, how deep we're permitted to go.
size_t ok_pub_max_stack = 256;

// timeout entries in the negative filename lookup cache
int ok_pub3_neg_cache_timeout = 360;
int ok_pub3_svc_neg_cache_timeout = 5;

// clean the cache out every 10 seconds
int ok_pub3_clean_cache_interval = 10;

// how long a getfile object lives in the cache (1 hour)
int ok_pub3_getfile_object_lifetime = 3600;

// amount of time a client has to gather all chunks (2 minutes)
int ok_pub3_chunk_lease_time = 120;

// largest possible file/chunk size (should be less than ok_axprt_ps)
size_t ok_pub3_max_datasz = 0x1000;

// the number of oustanding chunk requests to pubd
size_t ok_pub3_chunk_window_size = 5;

// compatibility mode
bool ok_pub2_compatibility_mode = false;

// yy buffer to use when parsing pub files
size_t ok_pub3_yy_buffer_size = 0x100000;

const char *ok_pub3_treestat_sentinel = ".treestat_sentinel";
const char *ok_pub3_treestat_heartbeat = ".treestat_heartbeat";
const char *ok_pub3_err_obj_key = "pub3_error";
time_t ok_pub3_profiler_interval_msec = 10;
size_t ok_pub3_profiler_buf_minsize = 0x100000; // 1MB
size_t ok_pub3_profiler_buf_maxsize = 0x8000000; // 128 MB

//
// pub3 constants
//
bool ok_pub3_json_strict_escaping = true;
int  ok_pub3_json_int_bitmax = 52;
size_t ok_pub3_recycle_limit_int = 1000;
size_t ok_pub3_recycle_limit_bindtab = 1000;
size_t ok_pub3_recycle_limit_dict = 1000;
size_t ok_pub3_recycle_limit_slot = 1000;

//
// Turn off SUIO recyling by default.  This is a trick to save time in
// malloc, but for production OKWS, we need memory more than CPU time;
// of course, we can turn this on via okws_config.
//
u_int ok_recycle_suio_limit = 0;

size_t ok_http_inhdr_buflen_big = 0x4000;   
size_t ok_http_inhdr_buflen_sml = 0x1000;
size_t ok_dflt_cgibuf_sz = 0x10000;
bool ok_http_parse_query_string = true;
bool ok_http_parse_cookies = true;

const char *ok_double_fmt_int_default = "%.16g";
const char *ok_double_fmt_ext_default = "%.10g";

static void
vec2vec (vec<const char *> *out, const vec<str> &in)
{
  for (size_t i = 0; i < in.size (); i++) {
    out->push_back (in[i].cstr());
  }
  out->push_back (NULL);
}

static void
get_cfg_path (vec<str> *v, const char *env_var, const char *cfg_path[])
{
  //
  // various paths to look through, in order
  //
  static rxx x (":");

  str d;
  if (env_var && (d = getenv (env_var))) {
    split (v, x, d);
  }
  for (const char **cp = cfg_path; *cp; cp++) {
    v->push_back (*cp);
  }
}

const char *ok_cfg_path[] = { ok_etc_dir1, ok_etc_dir2, 
			      etc1dir, etc2dir, etc3dir,
			      NULL };

str 
okws_etcfile (const char *f, const char *env_var, 
	      const char **cfg_path)
{
  vec<str> v1;
  vec<const char *> v2;

  if (cfg_path == NULL)
    cfg_path = ok_cfg_path;

  get_cfg_path (&v1, env_var, cfg_path);
  vec2vec (&v2, v1);

  return sfsconst_etcfile (f, v2.base ());
}

str 
okws_etcfile_required (const char *f, const char *env_var, bool d,
		       const char **cfg_path) 
{ 
  vec<str> v1;
  vec<const char *> v2;

  if (cfg_path == NULL)
    cfg_path = ok_cfg_path;

  get_cfg_path (&v1, env_var, cfg_path);
  vec2vec (&v2, v1);

  return sfsconst_etcfile_required (f, v2.base (), d);
}

gzip_mode_t
ok_gzip_str_to_mode (const str &s, bool *okp)
{
  gzip_mode_t m = GZIP_NONE;
  bool ok = true;
  if (!s) {
    /* nothing */
  } else if (s == "1" || s == "on" || s == "smart") {
    m = GZIP_SMART;
  } else if (s == "0" || s == "off") {
    m = GZIP_NONE;
  } else if (s == "naive") {
    m = GZIP_NAIVE;
  } else {
    ok = false;
  }

  if (okp) *okp = ok;
  return m;
}
