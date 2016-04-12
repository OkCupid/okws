/* $Id$ */

%#include "pub3prot.h"
%#include "okprotext.h"

struct xpub_errdoc_t {
  int status;
  xpub_fn_t fn;
};

struct xpub_errdoc_set_t {
  xpub_errdoc_t docs<>;
};

/* A given service proc, which is indentifier by a service names
 * and brother-ID.  In most cases the brother-ID will be 0 for the
 * one and only service.  -1 means don't care about which, and
 * >0 specifies a particular service id.
 */
struct oksvc_proc_t {
   string name<>;
   int brother_id;   // OR -1 if no ID known/specified
   int num_brothers; // OR -1 if unknown 
};

enum ok_set_typ_t {
  OK_SET_NONE = 0,
  OK_SET_SOME = 1,
  OK_SET_ALL = 2
};

union oksvc_procs_t switch (ok_set_typ_t typ) {
 case OK_SET_SOME:
   oksvc_proc_t procs<>;
 default:
   void;
};

struct okctl_send_msg_arg_t {
   string msg<>;
};

union okctl_send_msg_res_t switch (bool ok) {
case TRUE:
   void;
case FALSE:
   string err<>;
};

struct oksvc_status_t {
   oksvc_proc_t proc;
   int pid;
   unsigned n_served;
   unsigned uptime;
};

struct oksvc_stats_t {
  unsigned hyper n_sent;
  unsigned hyper n_recv;
};

struct okctl_stats_t {
    oksvc_status_t status<>;
};

struct ok_custom_data_t {
   opaque data<>;
};

struct ok_custom_arg_t {
  oksvc_procs_t procs;
  ok_custom_data_t data;
};

union ok_custom_res_union_t switch (ok_xstatus_typ_t status) {
  case OK_STATUS_OK:
    ok_custom_data_t dat;
  default:
    void;
};

struct ok_custom_res_t {
  oksvc_proc_t             proc;
  ok_custom_res_union_t    res;
};

struct ok_custom_res_set_t {
  ok_custom_res_t results<>;
};

typedef ok_xstatus_typ_t okctl_sendcon_res_t;

struct ssl_ctx_t {
       string cipher<>;
       unsigned okd_ordinal;
};

typedef opaque okclnt_sin_t<>;

struct okctl_timespec_t {
       unsigned ts_sec;
       unsigned ts_nsec;
};

struct okctl_sendcon_arg2_t {
	opaque sin<>;
	unsigned port;
	ssl_ctx_t *ssl;
        okctl_timespec_t time_recv;
 	okctl_timespec_t time_sent;
	unsigned reqno; // >0 for keepalive connections
	opaque scraps<>; // leftover bytes passed back in keepalive
};

struct okssl_sendcon_arg_t {
       opaque sin<>;
       ssl_ctx_t ssl;
       int port;
};

typedef string ip_addr_t<16>;

struct okmgr_diagnostic_arg_t {
  oksvc_proc_t           proc;
  ok_diagnostic_domain_t domain;
  ok_diagnostic_cmd_t    cmd;
};

struct okctl_diagnostic_arg_t {
  ok_diagnostic_domain_t domain;
  ok_diagnostic_cmd_t    cmd;
};

enum oklog_file_t {
   OKLOG_NONE = 0,
   OKLOG_ACCESS = 1,
   OKLOG_ERROR = 2,
   OKLOG_SSL = 3
};

struct oklog_fast_arg_t {
  string access<>;
  string error<>;
  string ssl<>;
};

struct oklog_entry_t {
   oklog_file_t file;
   opaque data<>;
};

struct oklog_arg_t {
   oklog_entry_t entries<>;
};

struct oksvc_descriptor_t {
  oksvc_proc_t proc;
  int pid;
};

struct oksvc_reserve_arg_t {
  oksvc_proc_t proc;
  bool lazy;
};

struct okws_send_ssl_arg_t {
  int dummy;
};

struct okmgr_send_msg_arg_t {
   oksvc_procs_t procs;
   string msg<>;
};

struct emergency_kill_arg_t {
     oksvc_descriptor_t svc;
     int signal;
};

%#define LOG_IP     (1 << 0)
%#define LOG_UA     (1 << 1)
%#define LOG_SZ     (1 << 2)
%#define LOG_REQ    (1 << 3)
%#define LOG_SVC    (1 << 4)
%#define LOG_RFR    (1 << 5)
%#define LOG_UID    (1 << 6)

namespace RPC {

program OKCTL_PROGRAM {
	version OKCLT_VERS {
	
		void
		OKCTL_NULL (void) = 0;
	
		void
		OKCTL_READY (void) = 1;

		ok_xstatus_t
		OKCTL_CUSTOM_1_IN (ok_custom_arg_t) = 7;
	
		ok_xstatus_t 
		OKCTL_CUSTOM_1_OUT (ok_custom_data_t) = 8;

		ok_custom_res_set_t
		OKCTL_CUSTOM_2_IN (ok_custom_arg_t) = 9;

		ok_custom_data_t
		OKCTL_CUSTOM_2_OUT (ok_custom_data_t) = 10;

		xpub_errdoc_set_t
		OKCTL_REQ_ERRDOCS_2 (void) = 11;

		okctl_stats_t
		OKCTL_GET_STATS(void) = 12;

		void
		OKCTL_REENABLE_ACCEPT(void) = 14;

		oksvc_stats_t
		OKCTL_GET_STATS_FROM_SVC(void) = 15;

		ok_xstatus_typ_t
		OKCTL_DIAGNOSTIC(okctl_diagnostic_arg_t) = 16;

		okctl_sendcon_res_t
		OKCTL_SEND_CON2(okctl_sendcon_arg2_t) = 18;

		okctl_send_msg_res_t 
		OKCTL_SEND_MSG (okctl_send_msg_arg_t) = 19;

		okctl_sendcon_res_t
		OKCTL_KEEPALIVE(okctl_sendcon_arg2_t) = 20;

		void
		OKCTL_KILL (oksig_t) = 99;

	} = 1;
} = 11279;

program OKLOG_PROGRAM {
	version OKCTL_VERS {
	
		void
		OKLOG_NULL (void) = 0;

		bool
		OKLOG_LOG (oklog_arg_t) = 1;

		bool	
		OKLOG_TURN (void) = 2;

		unsigned
		OKLOG_GET_LOGSET (void) = 3;

		int
		OKLOG_CLONE (int) = 5;

		void
		OKLOG_KILL (ok_killsig_t) = 99;

	} = 1;
} = 11281;

program OKMGR_PROGRAM {
	version OKMGR_VERS {
		
		void
		OKMGR_NULL (void) = 0;

		ok_xstatus_t
		OKMGR_RELAUNCH (oksvc_procs_t) = 2;

		ok_xstatus_t
		OKMGR_TURNLOG (void) = 3;

		ok_xstatus_t
		OKMGR_CUSTOM_1 (ok_custom_arg_t) = 4;

		ok_custom_res_t
		OKMGR_CUSTOM_2 (ok_custom_arg_t) = 5;

		ok_xstatus_t
		OKMGR_DIAGNOSTIC(okmgr_diagnostic_arg_t) = 7;

		ok_xstatus_t
		OKMGR_SEND_MSG(okmgr_send_msg_arg_t) = 9;
	} = 1;
} = 11278;

program OKLD_PROGRAM {
	version OKLD_VERS {

		void
		OKLD_NULL(void) = 0;

		ok_xstatus_typ_t
		OKLD_NEW_SERVICE(oksvc_descriptor_t) = 1;

		ok_xstatus_typ_t
		OKLD_SEND_SSL_SOCKET(okws_send_ssl_arg_t) = 2;

		ok_xstatus_typ_t
		OKLD_POKE_LAZY_SERVICE(oksvc_descriptor_t) = 3;

		ok_xstatus_typ_t
		OKLD_RESERVE(oksvc_reserve_arg_t) = 4;

		ok_xstatus_typ_t
		OKLD_EMERGENCY_KILL(emergency_kill_arg_t) = 5;

	} = 1;

} = 11279;

program OKSSL_PROGRAM {
	version OKSSL_VERS {

		void
		OKSSL_NULL(void) = 0;

		ok_xstatus_typ_t 
		OKSSL_TOGGLE_ACCEPT(bool) = 1;

		ok_xstatus_typ_t
		OKSSL_NEW_CONNECTION(okssl_sendcon_arg_t) = 2;

	} = 1;
} = 11280;

program NULL_PROGRAM {
	version NULL_VERS {

		void
		NULL_NULL(void) = 0;

	} = 1;
} = 11281;

}; /* namespace RPC */
