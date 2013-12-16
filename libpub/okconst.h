// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Maxwell Krohn (max@okcupid.com)
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

#ifndef _LIBPUB_OKCONST_H
#define _LIBPUB_OKCONST_H 1

#include "async.h"
#include "sfs_select.h"

enum { XSSFILT_NONE = 0, XSSFILT_SOME = 1, XSSFILT_ALL = 2 };

//
// hard-coded upper and lower variable limits
//
#define OK_QMIN 0
#define OK_QMAX 8192
#define OK_PORT_MIN 10
#define OK_PORT_MAX 65000
#define OK_UID_MIN 100
#define OK_UID_MAX 65000
#define OK_RQSZLMT_MIN  size_t (1024)
#define OK_RQSZLMT_MAX  size_t (32*1024*1024)

#define OK_MAX_URI_LEN 128

#define OKD_FDS_HIGH_WAT_LL  0
#define OKD_FDS_HIGH_WAT_UL  0x100000
#define OKD_FDS_LOW_WAT_LL  0
#define OKD_FDS_LOW_WAT_UL  0x100000
#define OK_SVC_FD_QUOTA_LL 0
#define OK_SVC_FD_QUOTA_UL 0x100000
#define OK_RSL_LL 0
#define OK_RSL_UL 10240
#define OK_SVC_FD_HIGH_WAT_UL 102400
#define OK_SVC_FD_LOW_WAT_UL  100000

typedef enum { GZIP_NONE = 0, GZIP_SMART = 1, GZIP_NAIVE = 2 } gzip_mode_t;

//
// gzip parameters (see zstr.h)
//
extern gzip_mode_t ok_gzip_mode;
extern int     ok_gzip_compress_level;
extern int     ok_gzip_naive_compress_level;
extern u_int   ok_gzip_smallstr;
extern u_int   ok_gzip_cache_minstr;
extern u_int   ok_gzip_cache_maxstr;
extern u_int   ok_gzip_cache_storelimit;
extern u_int   ok_gzip_mem_level;

gzip_mode_t ok_gzip_str_to_mode (const str &s, bool *okp = NULL);

// Higher level gzip stuff
extern bool ok_gzip_chunking;                  // prefer chunked w/ gzip
extern bool ok_gzip_chunking_old_safaris;      // some safaris are broken
extern bool ok_gzip_error_pages;               // whether to gzip error pages

//
// user/group constants
//
extern const char *ok_pubd_uname;
extern const char *ok_pubd_gname;
extern const char *ok_okd_uname;
extern const char *ok_okd_gname;
extern const char *ok_logd_uname;
extern const char *ok_logd_gname;
extern const char *ok_ssl_uname;
extern const char *ok_ssl_gname;

//
// default packet size for all axprts
//
extern size_t ok_axprt_ps;

//
// port constants
//
extern u_int ok_pubd_port;
extern u_int ok_mgr_port;

//
// pub constatns
//
extern const char *ok_pubobjname;

//
// http constants
//
extern const char *ok_http_urlencoded;
extern const char *ok_http_multipart;
extern bool ok_http_parse_cookies;
extern bool ok_http_parse_query_string;

//
// configfile constants
//
extern const char *ok_pub_config;
extern const char *ok_etc_dir0;
extern const char *ok_etc_dir1;
extern const char *ok_etc_dir2;
extern const char *ok_etc_dir3;
extern const char *ok_okws_config;
extern const char *ok_okd_config;

// all dirs to search when looking for CFG files
extern const char *ok_cfg_path[];

//
// command line flag to use (by default) when passing an FDFD to
// a child. An FDFD is an FD used for sending other FDs over.
//
extern const char *ok_fdfd_command_line_flag;


//
// timeouts
//
extern u_int ok_connect_wait;
extern u_int ok_shutdown_timeout;
extern u_int ok_shutdown_timeout_fast;
extern u_int ok_shutdown_retries;
extern u_int ok_db_retries_max;
extern u_int ok_db_retry_delay;
extern u_int ok_demux_timeout;
extern u_int ok_ka_timeout;

//
// okd constants
//
extern u_int ok_listen_queue_max;
extern u_int ok_con_queue_max;
extern u_int ok_crashes_max;
extern u_int ok_csi;                           // crash sampling interval
extern const char *ok_version;
extern const char *ok_dname;                   // okd name == okd, usually
extern const char *ok_wsname;                  // okws name == okws, usually
extern u_int ok_dport;                         // okd port listen to
extern const char *ok_jaildir_top;             // for okld
extern const char *ok_topdir;                  // okws executables
extern const char *ok_coredumpdir;             // where to put coredumps
extern const char *ok_sockdir;                 // unix sockets available..
extern const char *ok_okdexecpath;             // where okd lives
extern const char *ok_jaildir_run;             // service rundir
extern const char *ok_service_bin;             // where service exes live
extern const char *ok_logd_pidfile;            // logfile for oklogd
extern u_int ok_resurrect_delay;               // when a child dies..
extern u_int ok_resurrect_delay_ms;            // in nanoseconds
extern u_int ok_chld_startup_time;             // how long startup takes
extern u_int ok_filter_cgi;                    // filter XSS stuff from cgi[]
extern u_int okd_fds_high_wat;                 // max FDs okd has outstanding
extern u_int okd_fds_low_wat;                  // below low wat turn accept on
extern u_int ok_svc_fds_high_wat;              // max FDs a svc has out
extern u_int ok_svc_fds_low_wat;               // below low wat turn accept on
extern u_int ok_svc_fd_quota;                  // FD quota for svc
extern bool okd_accept_msgs;                   // display messages re: accept()
extern bool ok_svc_accept_msgs;                // display messages re: accept()
extern u_int ok_ssdi;                          // syscall stat dump interval
extern bool ok_send_sin;                       // send sins over the fdchan?
extern const char *ok_mmcd;                    // path to the mmcd
extern const char *ok_mmc_file;                // mmap'ed clock file
extern bool okd_child_sel_disable;             // disable child selects
extern const char *okws_server_label;          // advertised server label
extern u_int okd_debug_msg_freq;               // =0 for no debug messages
extern int64_t okws_debug_flags;               // OKWS debug flags
extern bool ok_dangerous_zbufs;                // zbufs that don't hold str's.
extern int ok_svc_life_reqs;                   // Upper limit on Reqs per life
extern int ok_svc_life_time;                   // Upper limit on Svc lifetime
extern const char *okd_mgr_socket;             // okd's management socket
extern int okd_mgr_socket_mode;                // chown sock to this mode
extern u_int okd_accept_delay;                 // delay before enabling accept
extern bool okd_tcp_nodelay;                   // whether okd disables Nagle
extern const char *ok_coredump_user;           // whom to chown coredumps to...
extern const char *ok_coredump_group;          // whom to chgrp coredumps to...
extern int ok_coredump_mode;                   // what mode to chmod a dump to
extern size_t ok_max_brother_procs;            // max number of brother procs

extern bool ok_ahttpcon_zombie_warn;           // zombie warn
extern time_t ok_ahttpcon_zombie_timeout;      // how long to wait before warn 

extern time_t okd_emergency_kill_wait_time;    // time to emergency kill
extern int okd_emergency_kill_signal;          // signal to send
extern time_t okd_sendcon_time_budget;         // sending a con should be fast

 

//
// okld constants
//
extern u_int okld_startup_batch_size;          // number in each batch
extern u_int okld_startup_batch_wait;          // n secs to wait between batch

//
// okssl constants
//
extern const char *ok_ssl_certfile;           // where the cert is kept
extern const char *ok_ssl_keyfile;            // where the priv key is kept
extern u_int ok_ssl_timeout;                  // how long before we timeout
extern u_int ok_ssl_port;                     // 443

//
// helper constants
//
extern u_int hlpr_max_calls;                   // max outstanding calls
extern u_int hlpr_max_retries;                 // ... before giving up
extern u_int hlpr_retry_delay;                 // delay between retries.
extern u_int hlpr_max_qlen;                    // maximum # to q

//
// log constants
//
extern time_t ok_log_tick;                      // how often log timer ticks
extern size_t ok_log_period;                    // how often to flush (in ticks)
extern size_t ok_log_hiwat;                     // max # of entries to buffer
extern str ok_syslog_priority;                  // syslog section
extern str ok_syslog_tag;                       // syslog tag (-t format)
extern str ok_syslog_domain;                    // domain for syslog
extern str ok_access_log_fmt;                   // default log format

// 
// sfs system constants
//
extern sfs_core::select_policy_t ok_sys_sel_policy;

//
// service/client constants
//
extern u_int ok_clnt_timeout;                   // user timeout

// MK 3/16/09: All sizes should be size_t's, but start on these
// for now.
extern size_t ok_hdrsize_limit;                 // biggest HTTP header allowed
extern size_t ok_reqsize_limit;                 // maximum client req size
extern size_t ok_cgibuf_limit;                  // limit cgi buf for testing

//
// Async-Multi-Threaded stuff
//
extern u_int ok_amt_lasi;                       // load avg sampling interval
extern u_int ok_ldavg_rpc_timeout;              // load avg RPC timeout
extern u_int ok_ldavg_ping_interval;            // how often to qry for loadavg
extern u_int ok_lblnc_launch_timeout;           // launch timeout
extern u_int ok_amt_report_q_freq;              // report q frequency
extern u_int ok_amt_q_timeout;                  // timeout RPCs sitting in Q
extern u_int ok_amt_stat_freq;                  // statistics sampling freq
extern bool ok_amt_rpc_stats;                   // whether to turn on RPC stats
extern u_int ok_amt_rpc_stats_interval;         // interval to printf
extern bool ok_kthread_safe;                    // whether to make libs 
                                                //    kernel-thread-safe

//
// HTTP constant stuff
//
extern size_t ok_http_inhdr_buflen_big;         // headers need to fit in here
extern size_t ok_http_inhdr_buflen_sml;         // for sub-buffers
extern size_t ok_dflt_cgibuf_sz;                // for small cgi static buffers

//
// Service UID Ranges
//
extern int ok_svc_uid_low;
extern int ok_svc_uid_high;
extern int ok_svc_mode;
extern int ok_interpreter_mode;
extern int ok_script_mode;

//
// OK Web Client params
//
extern int okwc_def_contlen;                     // when servers don't give it
extern size_t okwc_scratch_sz;

//
// On most platforms these are 'root' and 'wheel' but not all!
//
extern const char *ok_root;
extern const char *ok_wheel;

//
// recycle suios and suiolites for performance (0 => no recycle)
//
extern u_int ok_recycle_suio_limit ;

//
// pub2 constants
//
extern int ok_pub3_wss;
extern int ok_pub3_refresh_min;
extern int ok_pub3_refresh_max;
extern int ok_pub3_refresh_incr;
extern int ok_pub3_caching;
extern int ok_pub3_viserr;
extern int ok_pub3_neg_cache_timeout;
extern int ok_pub3_clean_cache_interval;
extern int ok_pub3_svc_neg_cache_timeout;
extern int ok_pub3_getfile_object_lifetime;
extern int ok_pub3_chunk_lease_time;
extern int ok_pub3_treestat_interval;
extern size_t ok_pub3_max_datasz;
extern const char *ok_pub3_treestat_sentinel;
extern const char *ok_pub3_treestat_heartbeat;
extern size_t ok_pub_max_stack;
extern size_t ok_pub3_chunk_window_size;
extern bool ok_pub2_compatibility_mode;
extern const char *ok_pub3_err_obj_key;
extern time_t ok_pub3_profiler_interval_msec;
extern size_t ok_pub3_profiler_buf_minsize;
extern size_t ok_pub3_profiler_buf_maxsize;

//
// pub3 constants
//
extern bool ok_pub3_json_strict_escaping;
extern size_t ok_pub3_yy_buffer_size;
extern int ok_pub3_json_int_bitmax;
extern size_t ok_pub3_recycle_limit_int;
extern size_t ok_pub3_recycle_limit_bindtab;
extern size_t ok_pub3_recycle_limit_dict;
extern size_t ok_pub3_recycle_limit_slot;



/**
 * Find an OKWS configuration file in the standard config file search path
 *
 * @param f the file to find.
 * @param e look in the env var and prepend value to front of path
 * @param p the path by default to check (if NULL, use default)
 */
str okws_etcfile (const char *f, const char *e = NULL,
		  const char **p = NULL);

/**
 * Find an OKWS configuration file in the standard config file search path
 * and puke / call fatal if not found.
 *
 * @param f the file to find.
 * @param e look in the env var and prepend value to front of path
 * @param d whether to dump the program on error
 * @param p the path by default to check (if NULL, use default)
 */
str okws_etcfile_required (const char *f, const char *e = NULL, bool d = true,
			   const char **p = NULL);

// A default double format for printing and interchange of data.
extern const char *ok_double_fmt_int_default;  // for intnernal use
extern const char *ok_double_fmt_ext_default;  // for external use (display)


#endif /* _LIBPUB_OKCONST_H */
