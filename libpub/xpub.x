
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
  XPUB_INCLIST = 8
};

typedef opaque xpubhash_t[PUBHASHSIZE];
typedef opaque okauthtok_t[OKAUTHTOKSIZE];

enum oksig_t {
  OK_SIG_NONE = 0,
  OK_SIG_HARDKILL = 1,
  OK_SIG_SOFTKILL = 2,
  OK_SIG_KILL = 3,
  OK_SIG_ABORT = 4 
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

struct xpub_include_t {
  int lineno;
  xpub_fn_t fn;
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

struct xpub_switch_env_t {
  xpub_key_t *key;
  xpub_fn_t *fn;
  xpub_aarr_t aarr;
};

struct xpub_set_func_t {
  int lineno;
  xpub_aarr_t aarr;
};

struct xpub_switch_t {
  int lineno;
  xpub_switch_env_t cases<>;
  xpub_switch_env_t *defcase;
  xpub_switch_env_t *nullcase;
  xpub_var_t key;
  bool nulldef;
};

union xpub_obj_t switch (xpub_obj_typ_t typ) {
 case XPUB_NONE:
   void;
 case XPUB_INCLUDE:
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
   xpub_set_func_t set_func;
 case XPUB_INCLIST:
   xpub_inclist_t inclist;
};

enum xpub_status_typ_t {
  XPUB_STATUS_OK = 0,
  XPUB_STATUS_ERR = 1,
  XPUB_STATUS_NOCHANGE = 2,
  XPUB_STATUS_NOENT = 3
};

union xpub_status_t switch (xpub_status_typ_t status)
{
 case XPUB_STATUS_OK:
   void;
 default:
   string error<>;
};

struct xpub_result_t {
  xpub_status_t status;
  xpub_set_t set;
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

union xpub_getfile_res_t switch (xpub_status_typ_t status)
{
 case XPUB_STATUS_OK:
   xpub_file_t file;
 case XPUB_STATUS_ERR:
   string error<>;
 default:
   void;
};

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

		void
		PUB_KILL (ok_killsig_t) = 99;

	} = 1;
} = 11277;
