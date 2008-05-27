/* $Id$ */

%#include "xpub.h"

struct xpub_errdoc_t {
  int status;
  xpub_fn_t fn;
};

struct xpub_errdoc_set_t {
  xpub_errdoc_t docs<>;
};

/* BAD nomenclature; ok_prog_t should be ok_service_t; each
 * prog corresponds to a separate OK service
 */
typedef string ok_prog_t <>;

enum ok_set_typ_t {
  OK_SET_NONE = 0,
  OK_SET_SOME = 1,
  OK_SET_ALL = 2
};

union ok_progs_t switch (ok_set_typ_t typ) {
 case OK_SET_SOME:
   ok_prog_t progs<>;
 default:
   void;
};

struct oksvc_status_t {
   ok_prog_t servpath;
   int pid;
   unsigned n_served;
   unsigned uptime;
};

struct okctl_stats_t {
	oksvc_status_t status<>;
};

struct ok_custom_data_t {
   opaque data<>;
};

struct ok_custom_arg_t {
  ok_progs_t progs;
  ok_custom_data_t data;
};

enum ok_xstatus_typ_t {
  OK_STATUS_OK = 0,
  OK_STATUS_PUBERR = 1,
  OK_STATUS_NOSUCHCHILD = 2,
  OK_STATUS_ERR = 3,
  OK_STATUS_DEADCHILD = 4,
  OK_STATUS_NOMORE = 5,
  OK_STATUS_BADFD = 6
};

union ok_custom_res_union_t switch (ok_xstatus_typ_t status) {
  case OK_STATUS_OK:
    ok_custom_data_t dat;
  default:
    void;
};

struct ok_custom_res_t {
  ok_prog_t                prog;
  ok_custom_res_union_t    res;
};

struct ok_custom_res_set_t {
  ok_custom_res_t results<>;
};

union ok_xstatus_t switch (ok_xstatus_typ_t status) 
{
 case OK_STATUS_OK:
   void;
 default:
   string error<>;
};

typedef ok_xstatus_typ_t okctl_sendcon_res_t;

struct okctl_sendcon_arg_t {
	opaque sin<>;
};

typedef string ip_addr_t<16>;
enum oklog_typ_t {
  OKLOG_OK = 0,
  OKLOG_ERR_WARNING = 1,
  OKLOG_ERR_ERROR = 2,
  OKLOG_ERR_NOTICE = 3,
  OKLOG_ERR_CRITICAL = 4,
  OKLOG_ERR_DEBUG = 5
};

struct oklog_notice_t {
  string notice<>;
};

struct oklog_ok_t {
  int status;
  string req<>;
  ip_addr_t ip;
  int size;
  string user_agent<>;
  string service<>;
  string referer<>;
  hyper uid;
};

struct oklog_err_t {
  oklog_ok_t log;
  string aux<>;
};

union oklog_arg_t switch (oklog_typ_t typ) {
 case OKLOG_OK:
   oklog_ok_t ok;
 case OKLOG_ERR_NOTICE:
 case OKLOG_ERR_CRITICAL:
   oklog_notice_t notice;
 default:
   oklog_err_t err;    
};

struct oklog_fast_arg_t {
  string access<>;
  string error<>;
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
	
		void
		OKCTL_CUSTOM_1_OUT (ok_custom_data_t) = 8;

		ok_custom_res_set_t
		OKCTL_CUSTOM_2_IN (ok_custom_arg_t) = 9;

		ok_custom_data_t
		OKCTL_CUSTOM_2_OUT (ok_custom_data_t) = 10;

		xpub_errdoc_set_t
		OKCTL_REQ_ERRDOCS_2 (void) = 11;

		okctl_stats_t
		OKCTL_GET_STATS(void) = 12;

		okctl_sendcon_res_t
		OKCTL_SEND_CON(okctl_sendcon_arg_t) = 13;

		void
		OKCTL_REENABLE_ACCEPT(void) = 14;

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

		bool
		OKLOG_FAST (oklog_fast_arg_t) = 4;

		bool
		OKLOG_CLONE (void) = 5;
		
		void
		OKLOG_KILL (ok_killsig_t) = 99;
	} = 1;
} = 11281;

program OKMGR_PROGRAM {
	version OKMGR_VERS {
		
		void
		OKMGR_NULL (void) = 0;

		ok_xstatus_t
		OKMGR_RELAUNCH (ok_progs_t) = 2;

		ok_xstatus_t
		OKMGR_TURNLOG (void) = 3;

		ok_xstatus_t
		OKMGR_CUSTOM_1 (ok_custom_arg_t) = 4;

		ok_custom_res_t
		OKMGR_CUSTOM_2 (ok_custom_arg_t) = 5;
	} = 1;
} = 11278;

};
