// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Max Krohn (max@scs.cs.nyu.edu)
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

//
// hard-coded upper and lower variable limits
//
#define OK_QMIN 1
#define OK_QMAX 8192
#define OK_PORT_MIN 10
#define OK_PORT_MAX 65000
#define UID_MIN 100
#define UID_MAX 65000
#define OK_RQSZLMT_MIN  1024
#define OK_RQSZLMT_MAX  32*1024*1024

//
// gzip parameters (see zstr.h)
//
extern bool    ok_gzip;
extern int     ok_gzip_compress_level;
extern u_int   ok_gzip_smallstr;
extern u_int   ok_gzip_cache_minstr;
extern u_int   ok_gzip_cache_maxstr;
extern u_int   ok_gzip_cache_storelimit;
extern u_int   ok_gzip_mem_level;

//
// user/group constants
//
extern const char *ok_uname;
extern const char *ok_gname;
extern const char *ok_svc_uname;
extern const char *ok_svc_gname;

//
// port constants
//
extern u_int ok_pubd_port;
extern u_int ok_mgr_port;

//
// configfile constants
//
extern const char *ok_pub_config;

//
// timeouts
//
extern u_int ok_connect_wait;
extern u_int ok_shutdown_timeout;
extern u_int ok_db_retries_max;
extern u_int ok_db_retry_delay;

//
// okd constants
//
extern u_int ok_listen_queue_max;
extern u_int ok_con_queue_max;
extern u_int ok_crashes_max;
extern u_int ok_csi;                           // crash sampling interval
extern const char *ok_version;
extern const char *ok_dname;                   // okd name == okd, usually
extern u_int ok_dport;                         // okd port listen to
extern const char *ok_jaildir_top;             // for okld
extern const char *ok_topdir;                  // okws executables
extern const char *ok_coredumpdir;             // where to put coredumps
extern const char *ok_sockdir;                 // unix sockets available..
extern const char *ok_okdexecpath;             // where okd lives
extern const char *ok_jaildir_run;             // service rundir
extern const char *ok_service_bin;             // where service exes live
extern u_int ok_resurrect_delay;               // when a child dies..
extern u_int ok_resurrect_delay_ms;            // in nanoseconds
extern u_int ok_chld_startup_time;             // how long startup takes

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
extern u_int ok_log_tick;                       // how often log timer ticks
extern u_int ok_log_period;                     // how many ticks to a flush
extern str ok_syslog_priority;                  // syslog section

//
// service/client constants
//
extern u_int ok_clnt_timeout;                   // user timeout
extern u_int ok_reqsize_limit;                  // maximum client req size

//
// Async-Multi-Threaded stuff
//
extern u_int ok_amt_lasi;                       // load avg sampling interval
extern u_int ok_ldavg_rpc_timeout;              // load avg RPC timeout
extern u_int ok_ldavg_ping_interval;            // how often to qry for loadavg
extern u_int ok_lblnc_launch_timeout;           // launch timeout

//
// Service UID Ranges
//
extern int ok_svc_uid_low;
extern int ok_svc_uid_high;

//
// OK Web Client params
//
extern int okwc_def_contlen;                     // when servers don't give it


#endif /* _LIBPUB_OKCONST_H */
