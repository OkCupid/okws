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

struct xpub2_fstat_t {
  xpub_fn_t  fn;
  u_int32_t  ctime;
  xpub3_hash_t hash;
};

%struct xpub3_expr_t;

struct xpub_zstr_t {
  xpub_str_t s;
  opaque zs<>;
  int clev;
};

enum xpub3_expr_typ_t {
   XPUB3_EXPR_NULL,
   XPUB3_EXPR_NOT,
   XPUB3_EXPR_FN,
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
   XPUB3_EXPR_BOOL
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

struct xpub3_fn_t {
   int lineno;
   string name<>;
   xpub3_expr_list_t args;
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

struct xpub3_dict_t {
  int lineno;
  xpub3_binding_t entries<>;
};

struct xpub3_bindlist_t {
  int lineno;
  xpub3_binding_t bindings<>;
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

union xpub3_expr_t switch (xpub3_expr_typ_t typ) {
case XPUB3_EXPR_NULL:
     void;
case XPUB3_EXPR_NOT:
     xpub3_not_t xnot;
case XPUB3_EXPR_MATHOP:
     xpub3_mathop_t mathop;
case XPUB3_EXPR_FN:
     xpub3_fn_t fn;
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
};

/* ======================================================================= */
/* Pub3 File Structures */

struct xpub3_metdata_t {
  xpub_fn_t jailed_filename;
  xpub_fn_t real_filename;
  xpub3_hash_t hash;
};

struct xpub3_file_t {
  xpub3_metadata_t metadata;
  xpub3_zone_t root;
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

struct xpub3_include_t {
  int lineno;
  xpub3_expr_t file;
  xpub3_expr_t dict;
};

struct xpub3_if_clause_t {
  int lineno;
  xpub3_expr_t expr;
  xpub_section_t body;
};

struct xpub3_if_t {
  int lineno;
  xpub3_if_clause_t clauses<>;
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
   XPUB3_EXPR_STATEMENT = 9
};

%struct xpub3_zone_t;

union xpub3_statement_t switch (xpub3_statement_typ_t typ) {
 case XPUB3_STATEMENT_NONE:
   void;

 case XPUB3_STATEMENT_INCLUDE:
 case XPUB3_STATEMENT_LOAD:
   xpub3_include_t include;

 case XPUB3_STATEMENT_ZONE:
   xpub3_zone_t zone;

 case XPUB3_STATEMENT_FOR:
   xpub3_for_t for_statement;

 case XPUB3_STATEMENT_LOCALS:
 case XPUB3_STATEMENT_UNIVERSALS:
   xpub3_decls_t decls;

 case XPUB3_STATEMENT_IF:
   xpub3_if_t if_statement;
 
 case XPUB3_STATEMENT_PRINT:
   xpub3_print_t print;

 case XPUB3_EXPR_STATEMENT:
   xpub3_expr_statement_t expr_statement;
};

/* ======================================================================= */
/* PUB3 Zones */

struct zpub3_zone_html_t {
  int lineno;
  bool preserve_white_space;
  xpub3_zone_t zones<>;
};

struct xpub3_zone_text_t {
  int lineno;
  xpub3_zstr_t original_text;
  xpub3_zstr_t wss_text;
};

struct xpub3_zone_pub_t {
  int lineno;
  xpub3_statement_t statements<>;
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
    XPUB3_ZONE_PUB = 4
};

union xpub3_zone_t (xpub3_zone_typ_t typ) {
case XPUB3_ZONE_HTML:
   xpub3_zone_html_t html;
case XPUB3_ZONE_TEXT:
   xpub3_zone_text_t text;
case XPUB3_ZONE_INLINE:
   xpub3_zone_inline_expr_t zone_inline;
case XPUB3_ZONE_PUB:
   xpub3_zone_pub_t zone_pub;
};

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

union xpub2_lookup_res_t switch (xpub_status_typ_t status)
{
 case XPUB_STATUS_OK:
   xpub2_fstat_t stat;
 case XPUB_STATUS_ERR:
   string error<>;
 default:
   void;
};

struct xpub2_chunk_t {
  unsigned 		offset;
  opaque		data<>;
};

struct xpub2_chunkshdr_t {
  xpub3_hash_t            xdrhash;   /* a hash of the file's XDR repr */
  int                   leasetime; /* time until flush possibility from cache*/
  unsigned		datasize;  /* size of the XDR repr */
};

union xpub2_xfered_file_t switch (xpub2_xfer_mode_t mode) {
case XPUB_XFER_WHOLE:
  xpub3_file_t whole;
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
  xpub3_hash_t hash;
};

struct xpub2_getfile_arg_t {
  xpub_fn_t               filename;
  unsigned                options;
  xpub2_file_freshcheck_t fresh;
  unsigned		  maxsz;
};

struct xpub2_getchunk_arg_t {
  xpub3_hash_t hash;
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

