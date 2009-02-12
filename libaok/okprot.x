/* $Id$ */

%#include "xpub.h"
%#include "okprotext.h"

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
  ok_progs_t progs;
  ok_custom_data_t data;
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

typedef ok_xstatus_typ_t okctl_sendcon_res_t;

struct ssl_ctx_t {
       string cipher<>;
};

struct okctl_sendcon_arg_t {
	opaque sin<>;
	unsigned port;
	ssl_ctx_t *ssl;
};

struct okssl_sendcon_arg_t {
       opaque sin<>;
       ssl_ctx_t ssl;
       int port;
};

typedef string ip_addr_t<16>;
enum oklog_typ_t {
  OKLOG_OK = 0,
  OKLOG_ERR_WARNING = 1,
  OKLOG_ERR_ERROR = 2,
  OKLOG_ERR_NOTICE = 3,
  OKLOG_ERR_CRITICAL = 4,
  OKLOG_ERR_DEBUG = 5,
  OKLOG_SSL = 6
};

struct okmgr_leak_checker_arg_t {
  ok_prog_t prog;
  ok_leak_checker_cmd_t cmd;
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

struct oklog_ssl_msg_t {
  string ip<>;
  string cipher<>;
  string msg<>;
};

union oklog_arg_t switch (oklog_typ_t typ) {
 case OKLOG_OK:
   oklog_ok_t ok;
 case OKLOG_ERR_NOTICE:
 case OKLOG_ERR_CRITICAL:
   oklog_notice_t notice;
 case OKLOG_SSL:
   oklog_ssl_msg_t ssl;
 default:
   oklog_err_t err;    
};

struct oklog_fast_arg_t {
  string access<>;
  string error<>;
  string ssl<>;
};

struct okws_svc_descriptor_t {
  string name<>;
  int pid;
};

struct okws_svc_reserve_arg_t {
  string name<>;
  bool lazy;
};

struct okws_send_ssl_arg_t {
  int dummy;
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

		xpub_errdoc_set_t
		OKCTL_REQ_ERRDOCS (void) = 2;

		ok_xstatus_t
	        OKCTL_UPDATE (xpub_fnset_t) = 3;

		xpub_getfile_res_t
		OKCTL_GETFILE (xpubhash_t) = 4;

		xpub_lookup_res_t
		OKCTL_LOOKUP (xpub_fn_t) = 5;

		xpub_getfile_res_t
		OKCTL_PUBCONF (void) = 6;

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

		okctl_sendcon_res_t
		OKCTL_SEND_CON(okctl_sendcon_arg_t) = 13;

		void
		OKCTL_REENABLE_ACCEPT(void) = 14;

		oksvc_stats_t
		OKCTL_GET_STATS_FROM_SVC(void) = 15;

		ok_xstatus_typ_t
		OKCTL_LEAK_CHECKER(ok_leak_checker_cmd_t) = 16;

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
		OKMGR_REPUB (xpub_fnset_t) = 1;

		ok_xstatus_t
		OKMGR_RELAUNCH (ok_progs_t) = 2;

		ok_xstatus_t
		OKMGR_TURNLOG (void) = 3;

		ok_xstatus_t
		OKMGR_CUSTOM_1 (ok_custom_arg_t) = 4;

		ok_custom_res_t
		OKMGR_CUSTOM_2 (ok_custom_arg_t) = 5;

		ok_xstatus_t
		OKMGR_REPUB2 (xpub_fnset_t) = 6;

		ok_xstatus_t
		OKMGR_LEAK_CHECKER(okmgr_leak_checker_arg_t) = 7;
	} = 1;
} = 11278;

program OKLD_PROGRAM {
	version OKLD_VERS {

		void
		OKLD_NULL(void) = 0;

		ok_xstatus_typ_t
		OKLD_NEW_SERVICE(okws_svc_descriptor_t) = 1;

		ok_xstatus_typ_t
		OKLD_SEND_SSL_SOCKET(okws_send_ssl_arg_t) = 2;

		ok_xstatus_typ_t
		OKLD_POKE_LAZY_SERVICE(okws_svc_descriptor_t) = 3;

		ok_xstatus_typ_t
		OKLD_RESERVE(okws_svc_reserve_arg_t) = 4;

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
