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

#include "okconst.h"


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
const char *ok_okd_uname = "www";     // unprivileged service user
const char *ok_okd_gname = "www";     // unprivileged service group

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
const char *ok_pub_config = "/etc/sfs/pub_config";

//
// timeouts
//
u_int ok_connect_wait = 4;          // seconds
u_int ok_shutdown_timeout = 10;     // seconds
u_int ok_shutdown_retries = 3;      // n retries before giving up
u_int ok_db_retries_max = 100;
u_int ok_db_retries_delay = 3;
u_int ok_demux_timeout = 10;        // clients have 10 secs to make a REQ

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

// location of the memory-mapped clock daemon
const char *ok_mmcd = "/usr/local/lib/sfs/lite/mmcd";

// location of mmcd's memory mapped file
const char *ok_mmc_file = "/var/run/mmcd.mmf";

// what OKWS reports it is in HTTP responses
const char *okws_server_label = "OKWS/"VERSION;


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
str   ok_syslog_priority = "local3.info";
str   ok_access_log_fmt = "ivt1sbz";

//
// service/client constants
//
u_int ok_clnt_timeout = 60;          // in seconds
u_int ok_reqsize_limit = 2097152;    // 2MB
u_int ok_hdrsize_limit = 0x2000;     // 8K

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
int ok_svc_mode = 0410;

int okwc_def_contlen = 0x20000;      // 128K 
