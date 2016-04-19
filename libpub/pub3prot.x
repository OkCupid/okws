/* $Id$ */

%#define PUBHASHSIZE 20
%#define OKAUTHTOKSIZE 20

typedef opaque xpub3_hash_t[PUBHASHSIZE];
typedef opaque okauthtok_t[OKAUTHTOKSIZE];

enum oksig_t {
  OK_SIG_NONE = 0,
  OK_SIG_HARDKILL = 1,
  OK_SIG_SOFTKILL = 2,
  OK_SIG_KILL = 3,
  OK_SIG_ABORT = 4 
};

enum xpub_version_t {
  XPUB_V1 = 1,
  XPUB_V2 = 2,
  XPUB_V3 = 3
};

struct ok_killsig_t {
  oksig_t sig;
  okauthtok_t authtok;
};

typedef string xpub_var_t <>;
typedef string xpub_str_t <>;
typedef string xpub_key_t <>;
typedef string xpub_fn_t  <>;
typedef unsigned hyper ok_xtime_t;

struct xpub3_identifier_list_t {
    xpub_str_t params<>;
    xpub_str_t opt_params<>;
};

struct xpub3_fstat_t {
  xpub_fn_t  fn;
  u_int32_t  ctime;
  xpub3_hash_t hash;
};

%struct xpub3_expr_t;
%struct xpub3_zone_t;

struct xpub3_zstr_t {
  opaque s<>;
  opaque zs<>;
  int clev;
};

enum xpub3_expr_typ_t {
   XPUB3_EXPR_NULL,
   XPUB3_EXPR_NOT,
   XPUB3_EXPR_CALL,
   XPUB3_EXPR_RELATION,
   XPUB3_EXPR_DICT,
   XPUB3_EXPR_EQ,
   XPUB3_EXPR_DICTREF,
   XPUB3_EXPR_VECREF,
   XPUB3_EXPR_REF,
   XPUB3_EXPR_SHELL_STR,   
   XPUB3_EXPR_STR,
   XPUB3_EXPR_INT,
   XPUB3_EXPR_UINT,
   XPUB3_EXPR_DOUBLE,
   XPUB3_EXPR_LIST,
   XPUB3_EXPR_REGEX,
   XPUB3_EXPR_MATHOP,
   XPUB3_EXPR_ASSIGNMENT,
   XPUB3_EXPR_BOOL,
   XPUB3_EXPR_PUBNULL,
   XPUB3_EXPR_LAMBDA,
   XPUB3_EXPR_HEREDOC,
   XPUB3_EXPR_SCOPED_REF
};

enum xpub3_relop_t { XPUB3_REL_LT, XPUB3_REL_GT, XPUB3_REL_LTE, XPUB3_REL_GTE };

enum xpub3_mathop_opcode_t {
     XPUB3_MATHOP_NONE = 0,
     XPUB3_MATHOP_ADD = 1,
     XPUB3_MATHOP_SUBTRACT = 2,
     XPUB3_MATHOP_MULT = 4,
     XPUB3_MATHOP_MOD = 5,
     XPUB3_MATHOP_AND = 6,
     XPUB3_MATHOP_OR = 7,
     XPUB3_MATHOP_DIV = 8
};

struct xpub3_mathop_t {
   int lineno;
   xpub3_mathop_opcode_t opcode;
   xpub3_expr_t *o1;
   xpub3_expr_t *o2;
};

struct xpub3_not_t {
   int lineno;
   xpub3_expr_t *e;
};

struct xpub3_expr_list_t {
   int lineno;
   xpub3_expr_t list<>;
};

struct xpub3_call_t {
   int lineno;
   xpub3_expr_t *fn;
   xpub3_expr_list_t args;
   bool blocking;
};

struct xpub3_eq_t {
   int lineno;
   xpub3_expr_t *o1;
   xpub3_expr_t *o2;
   bool pos;
};

struct xpub3_relation_t {
   int lineno;
   xpub3_relop_t relop;
   xpub3_expr_t *left;
   xpub3_expr_t *right;
};

struct xpub3_dictref_t {
   int lineno;
   xpub3_expr_t *dict;
   string key<>;
};

struct xpub3_vecref_t {
   int lineno;
   xpub3_expr_t *vec;
   xpub3_expr_t *index;
};

struct xpub3_ref_t {
   int lineno;
   string key<>;
};

enum xpub3_ref_scope_t {
   XPUB3_REF_SCOPE_GLOBALS = 1,
   XPUB3_REF_SCOPE_UNIVERSALS = 2
};

struct xpub3_scoped_ref_t {
   int lineno;
   string key<>;
   xpub3_ref_scope_t scope;
};

struct xpub3_shell_str_t {
   int lineno;
   xpub3_expr_list_t elements;
};

struct xpub3_str_t {
   string val<>;
};

struct xpub3_int_t {
   hyper val;
};

struct xpub3_bool_t {
  int i;
};

struct xpub3_uint_t {
  unsigned hyper val;
};

struct xpub3_double_t {
   string val<>;
};

struct xpub3_binding_t {
  xpub_key_t key;
  xpub3_expr_t *val;
};

typedef xpub3_binding_t xpub3_bindings_t<>;

struct xpub3_dict_t {
  int lineno;
  xpub3_bindings_t entries;
};

struct xpub3_regex_t {
  int lineno;
  string body<>;
  string opts<>;
};

struct xpub3_assignment_t {
  int lineno;
  xpub3_expr_t *lhs;
  xpub3_expr_t *rhs;
};

struct xpub3_lambda_t {
   int lineno;
   xpub3_str_t filename;
   xpub_fn_t name;
   xpub3_identifier_list_t params;
   xpub3_zone_t *body;
};

struct xpub3_heredoc_t {
   int lineno;
   xpub3_zone_t *body;
};

union xpub3_expr_t switch (xpub3_expr_typ_t typ) {
case XPUB3_EXPR_NULL:
     void;
case XPUB3_EXPR_PUBNULL:
     void;
case XPUB3_EXPR_NOT:
     xpub3_not_t xnot;
case XPUB3_EXPR_MATHOP:
     xpub3_mathop_t mathop;
case XPUB3_EXPR_CALL:
     xpub3_call_t call;
case XPUB3_EXPR_RELATION:
     xpub3_relation_t relation;
case XPUB3_EXPR_DICT:
     xpub3_dict_t dict;
case XPUB3_EXPR_LIST:
     xpub3_expr_list_t list;
case XPUB3_EXPR_EQ:
     xpub3_eq_t eq;
case XPUB3_EXPR_DICTREF:
     xpub3_dictref_t dictref;
case XPUB3_EXPR_VECREF:
     xpub3_vecref_t vecref;
case XPUB3_EXPR_REF:
     xpub3_ref_t xref;
case XPUB3_EXPR_SCOPED_REF:
     xpub3_scoped_ref_t scoped_xref;
case XPUB3_EXPR_SHELL_STR:
     xpub3_shell_str_t shell_str;
case XPUB3_EXPR_STR:
     xpub3_str_t xstr;
case XPUB3_EXPR_INT:
     xpub3_int_t xint;
case XPUB3_EXPR_UINT:
     xpub3_uint_t xuint;
case XPUB3_EXPR_DOUBLE:
     xpub3_double_t xdouble;
case XPUB3_EXPR_REGEX:
     xpub3_regex_t regex;
case XPUB3_EXPR_ASSIGNMENT:
     xpub3_assignment_t assignment;
case XPUB3_EXPR_BOOL:
     xpub3_int_t xbool;
case XPUB3_EXPR_LAMBDA:
     xpub3_lambda_t lambda;
case XPUB3_EXPR_HEREDOC:
     xpub3_heredoc_t heredoc;
};

/* ======================================================================= */
/* PUB3 Zones */

%struct xpub3_statement_t;

struct xpub3_zone_html_t {
  int lineno;
  xpub3_zone_t zones<>;
};

struct xpub3_zone_text_t {
  int lineno;
  xpub3_zstr_t original_text;
  xpub3_zstr_t wss_text;
};

struct xpub3_zone_raw_t {
  int lineno;
  xpub3_zstr_t data;
};

struct xpub3_zone_pub_t {
  int lineno;
  xpub3_statement_t statements<>;
};

struct xpub3_zone_wss_boundary_t {
  int lineno;
  bool on;
  string tag<>;
};

struct xpub3_zone_inline_expr_t {
  int lineno;
  xpub3_expr_t expr;
};

enum xpub3_zone_typ_t {
    XPUB3_ZONE_NONE = 0,
    XPUB3_ZONE_HTML = 1,
    XPUB3_ZONE_TEXT = 2,
    XPUB3_ZONE_INLINE_EXPR = 3,
    XPUB3_ZONE_PUB = 4,
    XPUB3_ZONE_RAW = 5,
    XPUB3_ZONE_WSS_BOUNDARY = 6
};

union xpub3_zone_t switch (xpub3_zone_typ_t typ) {
case XPUB3_ZONE_HTML:
   xpub3_zone_html_t html;
case XPUB3_ZONE_TEXT:
   xpub3_zone_text_t text;
case XPUB3_ZONE_INLINE_EXPR:
   xpub3_zone_inline_expr_t zone_inline;
case XPUB3_ZONE_PUB:
   xpub3_zone_pub_t zone_pub;
case XPUB3_ZONE_RAW:
   xpub3_zone_raw_t zone_raw;
case XPUB3_ZONE_WSS_BOUNDARY:
   xpub3_zone_wss_boundary_t zone_wss_boundary;
default: 
   void;
};

/* ======================================================================= */
/* Pub3 File Structures */

struct xpub3_metadata_t {
  xpub_fn_t jailed_filename;
  xpub_fn_t real_filename;
  xpub3_hash_t hash; // The hash of the **raw unparsed file** on the disk
  ok_xtime_t ctime;
};

struct xpub3_file_t {
  xpub3_metadata_t metadata;
  xpub3_zone_t *root;
  unsigned opts;
};

/* ======================================================================= */
/* PUB3 statements */

struct xpub3_for_t {
  int lineno;
  xpub_var_t iter;
  xpub3_expr_t arr;
  xpub3_zone_t body;
  xpub3_zone_t *empty;
};

struct xpub3_while_t {
  int lineno;
  xpub3_expr_t cond;
  xpub3_zone_t body;
};

struct xpub3_include_t {
  int lineno;
  xpub3_expr_t file;
  xpub3_expr_t dict;
};

struct xpub3_if_clause_t {
  int lineno;
  xpub3_expr_t expr;
  xpub3_zone_t body;
};

struct xpub3_if_t {
  int lineno;
  xpub3_if_clause_t clauses<>;
};

struct xpub3_case_t {
  int lineno;
  xpub3_expr_t *key;
  xpub3_zone_t body;
};

typedef xpub3_case_t xpub3_cases_t<>;

struct xpub3_switch_t {
  int lineno;
  xpub3_expr_t key;
  xpub3_cases_t cases;
  xpub3_case_t *defcase;
};

struct xpub3_return_t {
  int lineno;
  xpub3_expr_t retval;
};

struct xpub3_break_t {
  int lineno;
};

struct xpub3_fndef_t {
  int lineno;
  string name<>;
  xpub3_lambda_t lambda;
};

struct xpub3_continue_t {
  int lineno;
};

struct xpub3_exit_t {
  int lineno;
};

struct xpub3_decls_t {
  int lineno;
  xpub3_dict_t decls;
};

struct xpub3_print_t {
   int lineno;
   xpub3_expr_list_t args;
};

struct xpub3_expr_statement_t {
  int lineno;
  xpub3_expr_t *expr;
};

struct xpub3_statement_zone_t {
  int lineno;
  xpub3_zone_t zone;
};

enum xpub3_statement_typ_t {
   XPUB3_STATEMENT_NONE = 0,	
   XPUB3_STATEMENT_INCLUDE = 1,
   XPUB3_STATEMENT_LOAD = 2,
   XPUB3_STATEMENT_ZONE = 3,
   XPUB3_STATEMENT_FOR = 4,
   XPUB3_STATEMENT_LOCALS = 5,
   XPUB3_STATEMENT_UNIVERSALS = 6,
   XPUB3_STATEMENT_IF = 7,
   XPUB3_STATEMENT_PRINT = 8,
   XPUB3_STATEMENT_EXPR = 9,
   XPUB3_STATEMENT_FNDEF = 10,
   XPUB3_STATEMENT_SWITCH = 11,
   XPUB3_STATEMENT_BREAK = 12,
   XPUB3_STATEMENT_RETURN = 13,
   XPUB3_STATEMENT_CONTINUE = 14,
   XPUB3_STATEMENT_GLOBALS = 15,
   XPUB3_STATEMENT_WHILE = 16,
   XPUB3_STATEMENT_EXIT = 17
};

%struct xpub3_zone_t;

union xpub3_statement_t switch (xpub3_statement_typ_t typ) {
 case XPUB3_STATEMENT_NONE:
   void;

 case XPUB3_STATEMENT_INCLUDE:
 case XPUB3_STATEMENT_LOAD:
   xpub3_include_t include;

 case XPUB3_STATEMENT_ZONE:
   xpub3_statement_zone_t zone;

 case XPUB3_STATEMENT_FOR:
   xpub3_for_t for_statement;

 case XPUB3_STATEMENT_WHILE:
   xpub3_while_t while_statement;

 case XPUB3_STATEMENT_LOCALS:
 case XPUB3_STATEMENT_GLOBALS:
 case XPUB3_STATEMENT_UNIVERSALS:
   xpub3_decls_t decls;

 case XPUB3_STATEMENT_IF:
   xpub3_if_t if_statement;
 
 case XPUB3_STATEMENT_PRINT:
   xpub3_print_t print;

 case XPUB3_STATEMENT_EXPR:
   xpub3_expr_statement_t expr_statement;

 case XPUB3_STATEMENT_SWITCH:
   xpub3_switch_t switch_statement;

 case XPUB3_STATEMENT_BREAK:
   xpub3_break_t break_statement;

 case XPUB3_STATEMENT_CONTINUE:
   xpub3_continue_t continue_statement;

 case XPUB3_STATEMENT_RETURN:
   xpub3_return_t return_statement;

 case XPUB3_STATEMENT_FNDEF:
   xpub3_fndef_t fndef;

 case XPUB3_STATEMENT_EXIT:
   xpub3_exit_t exit_statement;
};

/* ====================================================================== */
/* pure JSON */

enum xpub3_json_typ_t { XPUB3_JSON_ERROR,
     		      	XPUB3_JSON_BOOL,
     		        XPUB3_JSON_INT32,
			XPUB3_JSON_UINT32,
     		      	XPUB3_JSON_INT64,
			XPUB3_JSON_UINT64,
			XPUB3_JSON_DOUBLE,
			XPUB3_JSON_STRING,
			XPUB3_JSON_DICT,
			XPUB3_JSON_LIST, 
			XPUB3_JSON_NULL };

%struct xpub3_json_t;

typedef opaque xpub3_json_str_t<>;

struct xpub3_json_list_t {
       xpub3_json_t entries<>;
};

struct xpub3_json_pair_t {
       xpub3_json_str_t key;
       xpub3_json_t *value;
};

typedef xpub3_json_pair_t xpub3_json_pairs_t<>;

struct xpub3_json_dict_t {
       xpub3_json_pairs_t entries;
};

union xpub3_json_t switch (xpub3_json_typ_t typ) {
case XPUB3_JSON_BOOL:
     bool json_bool;
case XPUB3_JSON_INT32:
     int json_int32;
case XPUB3_JSON_UINT32:
     unsigned json_uint32;
case XPUB3_JSON_INT64:
     hyper json_int64;
case XPUB3_JSON_UINT64:
     unsigned hyper json_uint64;
case XPUB3_JSON_STRING:
     xpub3_json_str_t json_string;
case XPUB3_JSON_DICT:
     xpub3_json_dict_t json_dict;
case XPUB3_JSON_LIST:
     xpub3_json_list_t json_list;
case XPUB3_JSON_DOUBLE:
     xpub3_double_t json_double;
default:
     void;
};

/* ----------------------------------------------------------------------- */

/* XDR descriptions of constants -- poor man's reflection! */

struct rpc_int_constant_t {
   string name<>;
   int value;
};

typedef rpc_int_constant_t rpc_int_constants_t<>;

struct rpc_constant_set_t {
   rpc_int_constants_t progs;
   rpc_int_constants_t vers;
   rpc_int_constants_t procs;
   rpc_int_constants_t enums;
   rpc_int_constants_t pound_defs;
};

namespace rpc {
  program JSON_INTROSPECTION_PROG {
    version JSON_INTROSPECTION_V1 {

      rpc_constant_set_t 
      JSON_INTROSPECTION_FETCH_CONSTANTS(void) = 0;

    } = 1;
  } = 79921;
};

/* ----------------------------------------------------------------------- */

/* ======================================================================= */

enum xpub_status_typ_t {
  XPUB_STATUS_OK = 0,
  XPUB_STATUS_ERR = 1,
  XPUB_STATUS_NOCHANGE = 2,
  XPUB_STATUS_NOENT = 3,
  XPUB_STATUS_OOB = 4,                 /* out of bounds */
  XPUB_STATUS_NOT_IMPLEMENTED = 5,
  XPUB_STATUS_RPC_ERR = 6,
  XPUB_UNAVAILABLE = 7,                /* disabled at runtime */
  XPUB_STATUS_CORRUPTION = 8,
  XPUB_STATUS_EPARSE = 9,
  XPUB_STATUS_EIO = 10
};

enum xpub3_xfer_mode_t {
  XPUB_XFER_WHOLE = 0,
  XPUB_XFER_CHUNKED = 1
};

typedef string xpub3_errstr_t<>;
typedef xpub3_errstr_t xpub3_errstrs_t<>;

union xpub_status_t switch (xpub_status_typ_t status)
{
 case XPUB_STATUS_OK:
   void;
 case XPUB_STATUS_RPC_ERR:
   u_int32_t rpc_err;
 case XPUB_STATUS_NOENT:
 case XPUB_STATUS_EPARSE:
 case XPUB_STATUS_EIO:
   xpub3_errstrs_t errors;
 default:
   xpub3_errstr_t error;
};

union xpub3_lookup_res_t switch (xpub_status_typ_t status)
{
 case XPUB_STATUS_OK:
   xpub3_fstat_t stat;
 case XPUB_STATUS_ERR:
   string error<>;
 default:
   void;
};

struct xpub3_chunk_t {
  unsigned 		offset;
  opaque		data<>;
};

struct xpub3_chunkshdr_t {
  xpub3_hash_t          xdrhash;   /* a hash of the file's XDR repr */
  int                   leasetime; /* time until flush possibility from cache*/
  unsigned		datasize;  /* size of the XDR repr */
  xpub3_hash_t          dathash;   /* hash of the file's data */
};

union xpub3_xfered_file_t switch (xpub3_xfer_mode_t mode) {
case XPUB_XFER_WHOLE:
  xpub3_file_t whole;
case XPUB_XFER_CHUNKED:
  xpub3_chunkshdr_t chunked;  
};

enum xpub3_freshness_typ_t {
  XPUB3_FRESH_NONE = 0,
  XPUB3_FRESH_CTIME = 1,
  XPUB3_FRESH_HASH = 2
};

union xpub3_file_freshcheck_t switch (xpub3_freshness_typ_t mode) {
case XPUB3_FRESH_NONE:
  void;
case XPUB3_FRESH_CTIME:
  u_int32_t ctime;
case XPUB3_FRESH_HASH:
  xpub3_hash_t hash;
};

struct xpub3_getfile_arg_t {
  xpub_fn_t               filename;
  unsigned                options;
  xpub3_file_freshcheck_t fresh;
  unsigned		  maxsz;
};

struct xpub3_getchunk_arg_t {
  xpub3_hash_t hash;
  unsigned opts;
  unsigned offset;
  unsigned size;
};

union xpub3_getchunk_res_t switch (xpub_status_typ_t status) {
case XPUB_STATUS_OK:
  xpub3_chunk_t chunk;
case XPUB_STATUS_ERR:
  string error<>;
default:
  void;
};

union xpub3_getfile_res_t switch (xpub_status_typ_t code) {
case XPUB_STATUS_OK:
  xpub3_xfered_file_t file;
default:
  xpub_status_t error_status;
};

union xpub3_get_root_config_res_t switch (xpub_status_typ_t status) {
case XPUB_STATUS_OK:
   xpub_fn_t fn;
case XPUB_STATUS_ERR:
   string error<>;
default:
   void;
};

struct xpub3_fstat_set_t {
   unsigned timestamp;
   xpub3_fstat_t fstats<>;
   xpub_fn_t misses<>;
};

/*
 * All files in the delta set should be removed from the service's
 * cache, either because the disappered, or because they were updated.
 */
struct xpub3_delta_set_t {
   hyper serial;
   unsigned start;
   unsigned stop;
   xpub_fn_t files<>;
};

union xpub3_get_fstats_res_t switch (xpub_status_typ_t status) {
case XPUB_STATUS_OK:
  xpub3_fstat_set_t stats;
case XPUB_STATUS_ERR:
  string error<>;
default:
  void;
};

namespace rpc {

program PUB_PROG {
	/*
	 * Version 2 of Pub. Simplified protocol, and a simplified
	 * server, too.
	 */
	version PUB_VERS3 {

		void
		PUB3_NULL (void) = 0;

		xpub3_get_root_config_res_t
		PUB3_GET_ROOT_CONFIG (void) = 1;

		xpub3_getfile_res_t
		PUB3_GETFILE (xpub3_getfile_arg_t) = 2;

		/*
		 * Input an mtime, and get all changes since that
		 * mtime (or perhaps all fstat's if there was a mod
		 * since the given mtime).
		 */
		xpub3_get_fstats_res_t
		PUB3_GET_FSTATS (u_int32_t) = 3;

		/*
	  	 * If the file is too big to be returned all at once,
		 * we need to get it by chunks.
		 */
		xpub3_getchunk_res_t 
		PUB3_GETCHUNK(xpub3_getchunk_arg_t) = 8;

		/*
		 * for each service, pubd needs to send a socket pair
		 * end over a pipe.
		 */
 	  	int 	
		PUB3_CLONE (int) = 4;
		
		bool
		PUB3_PUSH_DELTAS(xpub3_delta_set_t) = 5;	

		bool
		PUB3_GET_PUSHES (void) = 6;

		/*
	 	 * Get a file hash, and ctime, but bypass NFS
		 * so go from pubd<->pubd.
		 */
		xpub3_lookup_res_t
		PUB3_LOOKUP (xpub_fn_t) = 7;

		void
		PUB3_KILL (ok_killsig_t) = 99;
	} = 3;

} = 11277;

};

