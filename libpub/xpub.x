/* $Id$ */

%#define PUBHASHSIZE 20
%#define OKAUTHTOKSIZE 20

enum xpub_obj_typ_t {
  XPUB_NONE = 0,
  XPUB_INCLUDE = 1,
  XPUB_SECTION = 2,
  XPUB_HTML_EL = 3,
  XPUB_FILE_PSTR = 4,
  XPUB_FILE_VAR = 5,
  XPUB_SWITCH = 6,
  XPUB_SET_FUNC = 7,
  XPUB_INCLIST = 8,
  XPUB_INCLUDE2 = 9,
  XPUB_RAW = 10,
  XPUB_SET_LOCAL_FUNC = 11,
  XPUB_LOAD = 12,
  XPUB_FOR = 13,
  XPUB_COND = 14
};

typedef opaque xpubhash_t[PUBHASHSIZE];
typedef opaque okauthtok_t[OKAUTHTOKSIZE];

enum xpub_switch_env_typ_t {
  XPUB_SWITCH_ENV_NULLKEY = 0,
  XPUB_SWITCH_ENV_EXACT = 1,
  XPUB_SWITCH_ENV_RXX = 2,
  XPUB_SWITCH_ENV_RANGE = 3
};

enum oksig_t {
  OK_SIG_NONE = 0,
  OK_SIG_HARDKILL = 1,
  OK_SIG_SOFTKILL = 2,
  OK_SIG_KILL = 3,
  OK_SIG_ABORT = 4 
};

enum xpub_version_t {
  XPUB_V1 =1,
  XPUB_V2 = 2
};

struct ok_killsig_t {
  oksig_t sig;
  okauthtok_t authtok;
};

enum xpub_sec_typ_t {
  XPUB_NO_SEC = 0,
  XPUB_HTML_SEC = 1
};

enum xpub_val_typ_t {
  XPUB_VAL_NULL = 0,
  XPUB_VAL_INT = 1,
  XPUB_VAL_PSTR = 2,
  XPUB_VAL_MARR = 3,
  XPUB_VAL_IARR = 4
};	

enum xpub_pstr_el_typ_t {
  XPUB_PSTR_NONE = 0,
  XPUB_PSTR_VAR = 1,
  XPUB_PSTR_STR = 2
};

enum xpub_req_typ_t {
  XPUB_REQ_FN = 0,
  XPUB_REQ_HASH = 1,
  XPUB_REQ_PBINDING = 2
};

typedef string xpub_var_t <>;
typedef string xpub_str_t <>;
typedef string xpub_key_t <>;
typedef string xpub_fn_t  <>;

struct xpub_pbinding_t {
  xpub_fn_t fn;
  xpubhash_t hash;
};

struct xpub2_fstat_t {
  xpub_fn_t  fn;
  u_int32_t  ctime;
  xpubhash_t hash;
};


union xpub_pstr_el_t switch (xpub_pstr_el_typ_t typ) {
 case XPUB_PSTR_VAR:
   xpub_var_t var;
 case XPUB_PSTR_STR:
   xpub_str_t str;
 default:
   void;
};


struct xpub_pstr_t {
  xpub_pstr_el_t els<>;
};

%struct xpub_val_t;
typedef xpub_val_t xpub_parr_mixed_t<>;

typedef opaque xpub_parr_char_t<>;
typedef int xpub_parr_int_t<>;
typedef unsigned int xpub_parr_uint_t<>;
typedef hyper xpub_parr_int64_t<>;

enum xpub_int_typ_t {
  XPUB_CHAR = 1,
  XPUB_INT = 2,
  XPUB_UINT = 3,
  XPUB_INT64 = 4,
  XPUB_INT16 = 5,
  XPUB_UINT16 = 6
};

union xpub_parr_t switch (xpub_int_typ_t typ)
{
 case XPUB_CHAR:
   xpub_parr_char_t chararr;
 case XPUB_INT:
   xpub_parr_int_t intarr;
 case XPUB_INT16:
   xpub_parr_int_t int16arr;
 case XPUB_UINT16:
   xpub_parr_int_t uint16arr;
 case XPUB_UINT:
   xpub_parr_uint_t uintarr;
 case XPUB_INT64:
   xpub_parr_int64_t hyperarr;
 default:
   void;
};

union xpub_val_t switch (xpub_val_typ_t typ)
{
 case XPUB_VAL_INT:
   int i;
 case XPUB_VAL_NULL:
   void;
 case XPUB_VAL_PSTR:
   xpub_pstr_t pstr;
 case XPUB_VAL_MARR:
   xpub_parr_mixed_t marr;
 case XPUB_VAL_IARR:
   xpub_parr_t iarr;
};


struct xpub_nvpair_t {
  xpub_key_t key;
  xpub_val_t val;
};

struct xpub_aarr_t {
  xpub_nvpair_t tab<>;
};

union xpub_include_fn_t switch (xpub_version_t vrs)
{
case XPUB_V1:
  xpub_fn_t v1;
case XPUB_V2:
  xpub_pstr_t v2;
};

struct xpub_include_t {
  int lineno;
  xpub_include_fn_t fn;
  xpub_aarr_t env;
};

struct xpub_inclist_t {
  int lineno;
  xpub_fn_t files<>;
};

%struct xpub_obj_t;
struct xpub_section_t {
  int lineno;
  xpub_sec_typ_t typ;
  xpub_obj_t els<>;
};

struct xpub_zstr_t {
  xpub_str_t s;
  opaque zs<>;
  int clev;
};

struct xpub_html_el_t {
  xpub_zstr_t data;
};

struct xpub_raw_t {
  opaque dat<>;
};

struct xpub_file_pstr_t {
  xpub_pstr_t pstr;
};

struct xpub_file_t {
  xpubhash_t hsh;
  xpub_section_t secs<>;
};

struct xpub_set_t {
  xpub_file_t files<>;
  xpub_pbinding_t bindings<>;
};

struct xpub_fnset_t {
  xpub_fn_t files<>;
  bool rebind;
};

struct xpub_file_var_t {
  int lineno;
  xpub_var_t var;
};

enum switch_case_typ_t {
  XPUB_SWITCH_NONE = 0,
  XPUB_SWITCH_FILE = 1,
  XPUB_SWITCH_NESTED_ENV = 2
};

union xpub_switch_env_body_t switch (switch_case_typ_t typ)
{
case XPUB_SWITCH_FILE:
  xpub_fn_t fn;
case XPUB_SWITCH_NESTED_ENV:
  xpub_section_t sec;
default:
 void;
};

struct xpub_switch_env_base_t {
  xpub_switch_env_body_t body;
  xpub_aarr_t aarr;
};

struct xpub_switch_env_exact_t {
  xpub_switch_env_base_t base;
  xpub_key_t key; /* Cannot be NULL */
};

struct xpub_regex_t {
  string rxx<>;
  string opts<>;
};

struct xpub_irange_t {
  hyper low;
  hyper hi;
};

struct x_double_t {
  hyper n;
  hyper d;
};

struct xpub_drange_t {
  x_double_t low;
  x_double_t hi;
};

struct xpub_urange_t {
  unsigned hyper low;
  unsigned hyper hi;
};

struct xpub_switch_env_rxx_t {
  xpub_switch_env_base_t base;
  xpub_regex_t rxx;
};

enum xpub_range_typ_t { XPUB_IRANGE, XPUB_DRANGE, XPUB_URANGE };

union xpub_range_t switch (xpub_range_typ_t typ) {
case XPUB_IRANGE:
  xpub_irange_t ir;
case XPUB_DRANGE:
  xpub_drange_t dr;
case XPUB_URANGE:
  xpub_urange_t ur;
};

struct xpub_switch_env_range_t {
  xpub_switch_env_base_t base;
  xpub_range_t range;
};

struct xpub_switch_env_nullkey_t {
  xpub_switch_env_base_t base;
};

union xpub_switch_env_union_t switch (xpub_switch_env_typ_t typ) {
case XPUB_SWITCH_ENV_NULLKEY:
     xpub_switch_env_nullkey_t nullkey;
case XPUB_SWITCH_ENV_EXACT:
     xpub_switch_env_exact_t exact;
case XPUB_SWITCH_ENV_RXX:
     xpub_switch_env_rxx_t rxx;
case XPUB_SWITCH_ENV_RANGE:
     xpub_switch_env_range_t range;
};

struct xpub_set_func_t {
  int lineno;
  xpub_aarr_t aarr;
};

struct xpub_switch_t {
  int lineno;
  xpub_switch_env_union_t cases<>;
  xpub_switch_env_union_t *defcase;
  xpub_switch_env_union_t *nullcase;
  xpub_var_t key;
  bool nulldef;
};

struct xpub3_for_t {
  int lineno;
  xpub_var_t iter;
  xpub_var_t arr;
  xpub_section_t body;
  xpub_section_t empty;
};

enum xpub3_expr_typ_t {
   XPUB3_EXPR_NULL,
   XPUB3_EXPR_AND,
   XPUB3_EXPR_OR,
   XPUB3_EXPR_NOT,
   XPUB3_EXPR_FN,
   XPUB3_EXPR_RELATION,
   XPUB3_EXPR_EQ,
   XPUB3_EXPR_DICTREF,
   XPUB3_EXPR_VECREF,
   XPUB3_EXPR_REF,
   XPUB3_EXPR_STR,
   XPUB3_EXPR_INT,
   XPUB3_EXPR_DOUBLE
};
enum xpub3_relop_t { XPUB3_REL_LT, XPUB3_REL_GT, XPUB3_REL_LTE, XPUB3_REL_GTE };

%struct xpub3_expr_t;

struct xpub3_and_t {
   xpub3_expr_t *f1;
   xpub3_expr_t *f2;   
};

struct xpub3_or_t {
   xpub3_expr_t *t1;
   xpub3_expr_t *t2;   
};

struct xpub3_not_t {
   xpub3_expr_t *e;
};

typedef xpub3_expr_t xpub3_expr_list_t<>;

struct xpub3_fn_t {
   int lineno;
   string name<>;
   xpub3_expr_list_t args;
};

struct xpub3_eq_t {
   bool neg;
   xpub3_expr_t *e1;
   xpub3_expr_t *e2;
};

struct xpub3_relation_t {
   xpub3_relop_t relop;
   xpub3_expr_t *left;
   xpub3_expr_t *right;
};

struct xpub3_dictref_t {
   xpub3_expr_t *dict;
   string key<>;
};

struct xpub3_vecref_t {
   xpub3_expr_t *vec;
   xpub3_expr_t *index;
};

struct xpub3_ref_t {
   string key<>;
};

struct xpub3_str_t {
   string val<>;
};

struct xpub3_int_t {
   hyper val;
};

struct xpub3_double_t {
   string val<>;
};

union xpub3_expr_t switch (xpub3_expr_typ_t typ) {
case XPUB3_EXPR_NULL:
     void;
case XPUB3_EXPR_AND:
     xpub3_and_t xand;
case XPUB3_EXPR_OR:
     xpub3_or_t xxor;
case XPUB3_EXPR_NOT:
     xpub3_not_t xnot;
case XPUB3_EXPR_FN:
     xpub3_fn_t fn;
case XPUB3_EXPR_RELATION:
     xpub3_relation_t relation;
case XPUB3_EXPR_EQ:
     xpub3_eq_t eq;
case XPUB3_EXPR_DICTREF:
     xpub3_dictref_t dictref;
case XPUB3_EXPR_VECREF:
     xpub3_vecref_t vecref;
case XPUB3_EXPR_REF:
     xpub3_ref_t xref;
case XPUB3_EXPR_STR:
     xpub3_str_t xstr;
case XPUB3_EXPR_INT:
     xpub3_int_t xint;
case XPUB3_EXPR_DOUBLE:
     xpub3_double_t xdouble;
};

struct xpub3_cond_clause_t {
  int lineno;
  xpub3_expr_t expr;
  xpub_section_t body;
};

struct xpub3_cond_t {
  int lineno;
  xpub3_cond_clause_t clauses<>;
};

union xpub_obj_t switch (xpub_obj_typ_t typ) {
 case XPUB_NONE:
   void;
 case XPUB_INCLUDE:
 case XPUB_INCLUDE2:
 case XPUB_LOAD:
   xpub_include_t include;
 case XPUB_SECTION:
   xpub_section_t section;
 case XPUB_HTML_EL:
   xpub_html_el_t html_el;
 case XPUB_FILE_PSTR:
   xpub_file_pstr_t file_pstr;
 case XPUB_FILE_VAR:
   xpub_file_var_t file_var;
 case XPUB_SWITCH:
   xpub_switch_t swtch;
 case XPUB_SET_FUNC:
 case XPUB_SET_LOCAL_FUNC:
   xpub_set_func_t set_func;
 case XPUB_INCLIST:
   xpub_inclist_t inclist;
 case XPUB_FOR:
   xpub3_for_t forloop;
 case XPUB_COND:
   xpub3_cond_t cond;
 case XPUB_RAW:
   xpub_raw_t raw;
};

enum xpub_status_typ_t {
  XPUB_STATUS_OK = 0,
  XPUB_STATUS_ERR = 1,
  XPUB_STATUS_NOCHANGE = 2,
  XPUB_STATUS_NOENT = 3,
  XPUB_STATUS_OOB = 4,                 /* out of bounds */
  XPUB_STATUS_NOT_IMPLEMENTED = 5,
  XPUB_STATUS_RPC_ERR = 6,
  XPUB_UNAVAILABLE = 7,                /* disabled at runtime */
  XPUB_STATUS_CORRUPTION = 8
};

enum xpub2_xfer_mode_t {
  XPUB_XFER_WHOLE = 0,
  XPUB_XFER_CHUNKED = 1
};

union xpub_status_t switch (xpub_status_typ_t status)
{
 case XPUB_STATUS_OK:
   void;
 case XPUB_STATUS_RPC_ERR:
   u_int32_t rpc_err;
 default:
   string error<>;
};

struct xpub_result_t {
  xpub_status_t status;
  xpub_set_t set;
};

typedef u_int32_t xpub_cookie_t;
struct xpub_set_summary_t {
   xpub_pbinding_t bindings<>;
   u_int nfiles;
   xpub_cookie_t cookie;
};

/* In the PUBFILES v2 protocol, there is an idea of a session, so that way
 * the files can be served over in packets, not a huge, massive message.
 */
struct xpub_result2_t {
   xpub_status_t status;
   xpub_set_summary_t set;
};

union xpub_lookup_res_t switch (xpub_status_typ_t status)
{
 case XPUB_STATUS_OK:
   xpubhash_t hash;
 case XPUB_STATUS_ERR:
   string error<>;
 default:
   void;
};

union xpub2_lookup_res_t switch (xpub_status_typ_t status)
{
 case XPUB_STATUS_OK:
   xpub2_fstat_t stat;
 case XPUB_STATUS_ERR:
   string error<>;
 default:
   void;
};

union xpub_getfile_res_t switch (xpub_status_typ_t status)
{
 case XPUB_STATUS_OK:
   xpub_file_t file;
 case XPUB_STATUS_ERR:
   string error<>;
 default:
   void;
};

struct xpub_files2_getfile_arg_t {
  xpub_cookie_t cookie;
  u_int fileno;
};


struct xpub2_chunk_t {
  unsigned 		offset;
  opaque		data<>;
};

struct xpub2_chunkshdr_t {
  xpubhash_t            xdrhash;   /* a hash of the file's XDR repr */
  int                   leasetime; /* time until flush possibility from cache*/
  unsigned		datasize;  /* size of the XDR repr */
};

union xpub2_xfered_file_t switch (xpub2_xfer_mode_t mode) {
case XPUB_XFER_WHOLE:
  xpub_file_t whole;
case XPUB_XFER_CHUNKED:
  xpub2_chunkshdr_t chunked;  
};


struct xpub2_getfile_data_t {
   xpub2_fstat_t       stat;
   xpub2_xfered_file_t file;
};

enum xpub2_freshness_typ_t {
  XPUB2_FRESH_NONE = 0,
  XPUB2_FRESH_CTIME = 1,
  XPUB2_FRESH_HASH = 2
};

union xpub2_file_freshcheck_t switch (xpub2_freshness_typ_t mode) {
case XPUB2_FRESH_NONE:
  void;
case XPUB2_FRESH_CTIME:
  u_int32_t ctime;
case XPUB2_FRESH_HASH:
  xpubhash_t hash;
};

struct xpub2_getfile_arg_t {
  xpub_fn_t               filename;
  unsigned                options;
  xpub2_file_freshcheck_t fresh;
  unsigned		  maxsz;
};

struct xpub2_getchunk_arg_t {
  xpubhash_t hash;
  unsigned opts;
  unsigned offset;
  unsigned size;
};

union xpub2_getchunk_res_t switch (xpub_status_typ_t status) {
case XPUB_STATUS_OK:
  xpub2_chunk_t chunk;
case XPUB_STATUS_ERR:
  string error<>;
default:
  void;
};

union xpub2_getfile_res_t switch (xpub_status_typ_t status) {
case XPUB_STATUS_OK:
  xpub2_getfile_data_t data;
case XPUB_STATUS_ERR:
  string error<>;
default:
  void;
};

union xpub2_get_root_config_res_t switch (xpub_status_typ_t status) {
case XPUB_STATUS_OK:
   xpub_fn_t fn;
case XPUB_STATUS_ERR:
   string error<>;
default:
   void;
};

struct xpub2_fstat_set_t {
   unsigned timestamp;
   xpub2_fstat_t fstats<>;
   xpub_fn_t misses<>;
};

/*
 * All files in the delta set should be removed from the service's
 * cache, either because the disappered, or because they were updated.
 */
struct xpub2_delta_set_t {
   hyper serial;
   unsigned start;
   unsigned stop;
   xpub_fn_t files<>;
};


union xpub2_get_fstats_res_t switch (xpub_status_typ_t status) {
case XPUB_STATUS_OK:
  xpub2_fstat_set_t stats;
case XPUB_STATUS_ERR:
  string error<>;
default:
  void;
};

namespace RPC {

program PUB_PROGRAM {
	version PUB_VERS {

		void 
		PUB_NULL (void) = 0;

		xpub_result_t
		PUB_FILES (xpub_fnset_t) = 1;

		xpub_getfile_res_t
		PUB_CONFIG (void) = 2;

		xpub_getfile_res_t
		PUB_GETFILE (xpubhash_t) = 3;

		xpub_lookup_res_t
		PUB_LOOKUP (xpub_fn_t) = 4;

		/*
 		 * PUB_FILES the original version gave all files as a 
		 * massive packet, that was often too big to suck in all
		 * at once.  PUB_FILES2 is a bit friendlier, and allows
	         * the client to suck over one file at a time. The next
		 * three RPCs allow this..
		 */
		xpub_result2_t 
		PUB_FILES2 (xpub_fnset_t) = 5;
	
		xpub_getfile_res_t 
		PUB_FILES2_GETFILE (xpub_files2_getfile_arg_t) = 6;

		xpub_status_typ_t
		PUB_FILES2_CLOSE(xpub_cookie_t) = 7;

		void
		PUB_KILL (ok_killsig_t) = 99;

	} = 1;

	/*
	 * Version 2 of Pub. Simplified protocol, and a simplified
	 * server, too.
	 */
	version PUB_VERS2 {

		void
		PUB2_NULL (void) = 0;

		xpub2_get_root_config_res_t
		PUB2_GET_ROOT_CONFIG (void) = 1;

		xpub2_getfile_res_t
		PUB2_GETFILE (xpub2_getfile_arg_t) = 2;

		/*
		 * Input an mtime, and get all changes since that
		 * mtime (or perhaps all fstat's if there was a mod
		 * since the given mtime).
		 */
		xpub2_get_fstats_res_t
		PUB2_GET_FSTATS (u_int32_t) = 3;

		/*
	  	 * If the file is too big to be returned all at once,
		 * we need to get it by chunks.
		 */
		xpub2_getchunk_res_t 
		PUB2_GETCHUNK(xpub2_getchunk_arg_t) = 8;

		/*
		 * for each service, pubd needs to send a socket pair
		 * end over a pipe.
		 */
 	  	bool
		PUB2_CLONE (void) = 4;
		
		bool
		PUB2_PUSH_DELTAS(xpub2_delta_set_t) = 5;	

		bool
		PUB2_GET_PUSHES (void) = 6;

		/*
	 	 * Get a file hash, and ctime, but bypass NFS
		 * so go from pubd<->pubd.
		 */
		xpub2_lookup_res_t
		PUB2_LOOKUP (xpub_fn_t) = 7;

		void
		PUB2_KILL (ok_killsig_t) = 99;
	} = 2;

} = 11277;

};

