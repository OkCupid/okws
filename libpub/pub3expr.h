// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#ifndef _LIBPUB_PUB3DATA_H_
#define _LIBPUB_PUB3DATA_H_

#include "pub.h"
#include "parr.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class expr_t;
  class expr_regex_t;

  //-----------------------------------------------------------------------

  class json {
  public:
    static str null() { return _null; }
    static str quote (const str &s);
    static str safestr (const str &s);
    static str _null;
  };

  //-----------------------------------------------------------------------

  class eval_t {
  public:

    enum { EVAL_INIT = -2, EVAL_DONE = -1 };

    eval_t (penv_t *e, output_t *o); ;

    penv_t *penv () const { return _env; }
    output_t *output () const { return _output; }

    bool set_loud (bool b);
    bool loud () const { return _loud && !_silent; }
    bool set_silent (bool b);

    cache_generation_t cache_generation () const 
    { return _env->cache_generation (); }

    ptr<const pval_t> resolve (const expr_t *e, const str &nm);

    ptr<pval_t> eval_freeze (ptr<const pval_t> in);
    void eval_freeze_dict (const aarr_t *in, aarr_t *out);
    void eval_freeze_vec (const vec_iface_t *in, vec_iface_t *out);

    void set_in_json () { _in_json = true; }
    bool in_json () const { return _in_json; }

  private:

    penv_t *_env;
    output_t *_output;
    bool _loud;
    bool _silent;
    ssize_t _stack_p;
    bool _in_json;
  };

  //-----------------------------------------------------------------------

  class expr_t : public pval_t {
  public:
    expr_t (int lineno = -1) : _lineno (lineno) {}
    virtual ~expr_t () {}

    enum { max_stack_depth = 128,
	   max_shell_strlen = 0x100000 };

    virtual bool to_xdr (xpub3_expr_t *x) const = 0;
    bool to_xdr (xpub_obj_t *x) const;
    bool to_xdr (xpub_val_t *x) const;

    static ptr<expr_t> alloc (const xpub3_expr_t &x);
    static ptr<expr_t> alloc (const xpub3_expr_t *x);
    static ptr<expr_t> alloc (scalar_obj_t so);

    //------- Evaluation ZOO --------------------------------------
    //
    // The main evaluation system; evalute an expression completely
    // so that the object is a scalar that's fit for output or
    // for immediate evaluation.
    //
    virtual ptr<const pval_t> eval (eval_t e) const = 0;

    //
    // Evaluate, freezing all references in place, but maintaining
    // the same object structure.
    //
    virtual ptr<pval_t> eval_freeze (eval_t e) const = 0;

    //
    //------------------------------------------------------------

    //------------------------------------------------------------
    //
    // Once objects are evaluated, they can be turned into....

    virtual ptr<const aarr_t> to_dict () const { return NULL; }
    virtual ptr<const vec_iface_t> to_vec_iface () const { return NULL; }

    virtual scalar_obj_t to_scalar () const { return scalar_obj_t (); }
    virtual str to_identifier () const { return NULL; }
    virtual str to_str () const { return NULL; }
    virtual bool to_bool () const { return false; }
    virtual int64_t to_int () const { return 0; }
    virtual u_int64_t to_uint () const { return 0; }
    virtual bool to_len (size_t *s) const { return false; }
    virtual bool is_null () const { return false; }
    virtual ptr<rxx> to_regex () const { return NULL; }
    virtual ptr<expr_regex_t> to_regex_obj () { return NULL; }
    
    //
    // and from here, scalars can be converted at will...
    //
    //-----------------------------------------------------------

    //-----------------------------------------------------------
    //
    // So that pub v3 objects works in pub v1 and pub v2
    //

    void eval_obj (pbuf_t *ps, penv_t *e, u_int d) const;
    ptr<pub_scalar_t> to_pub_scalar ();
    ptr<const pub_scalar_t> to_pub_scalar () const;

    //
    //-----------------------------------------------------------

    //-----------------------------------------------------------
    // in pub2::output_conf2_t::output_set_func, there is an attempt
    // made to flatten objects.  for pub3, objects have already been
    // flattened, so just noop
    // 

    ptr<pval_t> set_func_flatten (penv_t *e) { return mkref (this); }

    //
    //
    //-----------------------------------------------------------


    //-----------------------------------------------------------
    //
    // Shortcuts follow, which for now do the long thing. They
    // might eventually do the short thing, by eliminating intermediary
    // objects.
    //

    ptr<const aarr_t> eval_as_dict (eval_t e) const;
    ptr<const vec_iface_t> eval_as_vec (eval_t e) const;
    virtual scalar_obj_t eval_as_scalar (eval_t e) const;
    bool eval_as_vec_or_dict (eval_t e, ptr<const vec_iface_t> *vp, 
			      ptr<const aarr_t> *dp) const;
    virtual ptr<rxx> eval_as_regex (eval_t e) const { return NULL; }

    virtual bool eval_as_bool (eval_t e) const;
    virtual int64_t eval_as_int (eval_t e) const;
    virtual u_int64_t eval_as_uint (eval_t e) const;
    virtual str eval_as_str (eval_t e) const;
    virtual bool eval_as_null (eval_t e) const { return false; }

    //
    //----------------------------------------------------------
    
    void report_error (eval_t e, str n) const;
    
    //------------------------------------------------------------
    //
    // For pub v1/v2 compatibility
    
    ptr<expr_t> to_expr () { return mkref (this); }
    ptr<const expr_t> to_expr () const { return mkref (this); }

    //
    //-----------------------------------------------------------------------

  protected:
    ptr<rxx> str2rxx (const eval_t *e, const str &b, const str &o) const;

    int _lineno;

    mutable cache_generation_t _cache_generation;
    mutable scalar_obj_t _so;
    mutable ptr<expr_t> _cached_result;
  };
  
  //-----------------------------------------------------------------------
  
  // Expressions for which the frozen repr is the same as the normal repr.
  class expr_frozen_t : public expr_t {
  public:
    expr_frozen_t (int l = -1) : expr_t (l) {}
    ptr<pval_t> eval_freeze (eval_t e) const;
  };

  //-----------------------------------------------------------------------
  
  // Expressions that can be evaluated immediately.
  class expr_static_t : public expr_frozen_t {
  public:
    expr_static_t (int l = -1) : expr_frozen_t (l) {}
    ptr<const pval_t> eval (eval_t e) const { return mkref (this); }

    scalar_obj_t eval_as_scalar (eval_t e) const { return to_scalar (); }
    virtual str eval_as_str (eval_t e) const { return to_str (); }
    int64_t eval_as_int (eval_t e) const { return to_int (); }
    u_int64_t eval_as_uint (eval_t e) const { return to_uint (); }
  };

  //-----------------------------------------------------------------------

  class expr_null_t : public expr_static_t {
  public:
    expr_null_t (int l = -1) : expr_static_t (l) {}
    bool is_null () const { return true; }
    bool to_xdr (xpub3_expr_t *x) const { return false; }
    const char *get_obj_name () const { return "pub3::expr_null_t"; }
    bool eval_as_null (eval_t e) const { return true; }
  };

  //-----------------------------------------------------------------------

  class expr_bool_t : public expr_static_t {
  public:
    expr_bool_t (bool b) : expr_static_t (), _b (b) {}
    scalar_obj_t to_scalar () const;
    bool to_xdr (xpub3_expr_t *x) const { return false; }
    str to_str () const;
    int64_t to_int () const { return _b; }
    u_int64_t to_uint () const { return _b; }
    const char *get_obj_name () const { return "pub3::expr_bool_t"; }
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
  private:
    bool _b;
  };

  //-----------------------------------------------------------------------

  class expr_logical_t : public expr_frozen_t {
  public:
    expr_logical_t (int l) : expr_frozen_t (l) {}
    ptr<const pval_t> eval (eval_t e) const;
  private:
    virtual bool eval_internal (eval_t e) const = 0;
    mutable bool _cached_bool;
    mutable ptr<expr_t> _cached_val;
  };

  //-----------------------------------------------------------------------

  class expr_OR_t : public expr_logical_t {
  public:
    expr_OR_t (ptr<expr_t> t1, ptr<expr_t> t2, int l) 
      : expr_logical_t (l), 
	_t1 (t1), 
	_t2 (t2) {}

    expr_OR_t (const xpub3_or_t &x);

    bool to_xdr (xpub3_expr_t *x) const;
    const char *get_obj_name () const { return "pub3::expr_OR_t"; }
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
  protected:
    ptr<expr_t> _t1, _t2;
    bool eval_internal (eval_t e) const;
  };

  //-----------------------------------------------------------------------

  class expr_pipe_t : public expr_frozen_t {
  public:
    expr_pipe_t (ptr<expr_t> l, ptr<expr_t> r, int lineno)
      : expr_frozen_t (lineno), _left (l), _right (r) {}
    expr_pipe_t (const xpub3_pipe_t &x);
    const char *get_obj_name () const { return "pub3::expr_pipe_t"; }
    bool to_xdr (xpub3_expr_t *x) const;

    ptr<const pval_t> eval (eval_t e) const { return NULL; }

  protected:
    ptr<expr_t> _left, _right;
  };

  //-----------------------------------------------------------------------

  class expr_AND_t : public expr_logical_t  {
  public:
    expr_AND_t (ptr<expr_t> f1, ptr<expr_t> f2, int lineno) 
      : expr_logical_t (lineno), _f1 (f1), _f2 (f2) {}

    expr_AND_t (const xpub3_and_t &x);
    bool to_xdr (xpub3_expr_t *x) const;
    const char *get_obj_name () const { return "pub3::expr_AND_t"; }
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
  protected:
    ptr<expr_t> _f1, _f2;
    bool eval_internal (eval_t e) const;
  };

  //-----------------------------------------------------------------------

  class expr_NOT_t : public expr_logical_t  {
  public:
    expr_NOT_t (ptr<expr_t> e, int lineno) 
      : expr_logical_t (lineno), _e (e) {}

    expr_NOT_t (const xpub3_not_t &x);
    bool to_xdr (xpub3_expr_t *x) const;
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    const char *get_obj_name () const { return "pub3::expr_NOT_t"; }
  protected:
    ptr<expr_t> _e;
    bool eval_internal (eval_t e) const;
  };

  //-----------------------------------------------------------------------

  class expr_EQ_t : public expr_logical_t {
  public:
    expr_EQ_t (ptr<expr_t> o1, ptr<expr_t> o2, bool pos, int ln) 
      : expr_logical_t (ln), _o1 (o1), _o2 (o2), _pos (pos) {}
    expr_EQ_t (const xpub3_eq_t &x);

    bool to_xdr (xpub3_expr_t *x) const;
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    const char *get_obj_name () const { return "pub3::expr_EQ_t"; }
  protected:
    ptr<expr_t> _o1, _o2;
    bool _pos;
    bool eval_internal (eval_t e) const;
  };

  //-----------------------------------------------------------------------

  class expr_relation_t : public expr_logical_t {
  public:
    expr_relation_t (ptr<expr_t> l, ptr<expr_t> r, xpub3_relop_t op, int lineno)
      : expr_logical_t (lineno), _l (l), _r (r), _op (op) {}
    expr_relation_t (const xpub3_relation_t &x);

    bool to_xdr (xpub3_expr_t *x) const;
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    const char *get_obj_name () const { return "pub3::expr_relation_t"; }

  protected:
    ptr<expr_t> _l, _r;
    xpub3_relop_t _op;
    bool eval_internal (eval_t e) const;
  };

  //-----------------------------------------------------------------------

  class expr_arithmetic_t : public expr_frozen_t {
  public:
    expr_arithmetic_t (int l) : expr_frozen_t (l) {}

    ptr<const pval_t> eval (eval_t e) const;
    void dump2 (dumper_t *d) const { /* XXX implement me */ }

    scalar_obj_t eval_as_scalar (eval_t e) const;
    int64_t eval_as_int (eval_t e) const;
    u_int64_t eval_as_uint (eval_t e) const;
    bool eval_as_bool (eval_t e) const;
    str eval_as_str (eval_t e) const;

  protected:
    virtual scalar_obj_t eval_internal (eval_t e) const = 0;
  };

  //-----------------------------------------------------------------------

  class expr_add_t : public expr_arithmetic_t {
  public:
    expr_add_t (ptr<expr_t> t1, ptr<expr_t> t2, bool pos, int lineno)
      : expr_arithmetic_t (lineno), _t1 (t1), _t2 (t2), _pos (pos) {}
    expr_add_t (const xpub3_add_t &x);

    bool to_xdr (xpub3_expr_t *x) const;
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    const char *get_obj_name () const { return "pub3::expr_add_t"; }
  protected:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _t1, _t2;
    bool _pos;
  };

  //-----------------------------------------------------------------------

  class expr_mod_t : public expr_arithmetic_t {
  public:
    expr_mod_t (ptr<expr_t> n, ptr<expr_t> d, int lineno)
      : expr_arithmetic_t (lineno), _n (n), _d (d) {}
    expr_mod_t (const xpub3_mod_t &x);

    bool to_xdr (xpub3_expr_t *x) const;
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    const char *get_obj_name () const { return "pub3::expr_add_t"; }
  protected:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _n, _d;
    bool _pos;
  };

  //-----------------------------------------------------------------------

  class expr_ref_t : public expr_t {
  public:
    expr_ref_t (int l) : expr_t (l) {}
    void dump2 (dumper_t *d) const { /* XXX implement me */ }

    ptr<const pval_t> eval (eval_t e) const;
    ptr<pval_t> eval_freeze (eval_t e) const;
    str eval_as_str (eval_t e) const;
    bool eval_as_null (eval_t e) const;
    ptr<rxx> eval_as_regex (eval_t e) const;

    ptr<const expr_ref_t> to_ref () const { return mkref (this); }

  protected:
    ptr<const pval_t> deref (eval_t *e) const;
    virtual ptr<const pval_t> deref_step (eval_t *e) const = 0;
    ptr<const pval_t> eval_internal (eval_t e) const;

  };

  //-----------------------------------------------------------------------

  class expr_dictref_t : public expr_ref_t {
  public:
    expr_dictref_t (ptr<expr_t> d, const str &k, int lineno)
      : expr_ref_t (lineno), _dict (d), _key (k) {}
    expr_dictref_t (const xpub3_dictref_t &x);
    bool to_xdr (xpub3_expr_t *x) const;
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    const char *get_obj_name () const { return "pub3::expr_dictref_t"; }

  protected:
    ptr<const pval_t> deref_step (eval_t *e) const ;
    ptr<expr_t> _dict;
    str _key;
  };

  //-----------------------------------------------------------------------

  class expr_vecref_t : public expr_ref_t {
  public:
    expr_vecref_t (ptr<expr_t> v, ptr<expr_t> i, int l) 
      : expr_ref_t (l), _vec (v), _index (i) {}
    expr_vecref_t (const xpub3_vecref_t &x);
    void dump2 (dumper_t *d) const { /* XXX implement me */ }

    bool to_xdr (xpub3_expr_t *x) const;
    const char *get_obj_name () const { return "pub3::expr_vecref_t"; }
  protected:
    ptr<const pval_t> deref_step (eval_t *e) const;

    ptr<expr_t> _vec;
    ptr<expr_t> _index;
  };
    
  //-----------------------------------------------------------------------

  class expr_varref_t : public expr_ref_t {
  public:
    expr_varref_t (const str &s, int l) : expr_ref_t (l), _name (s) {}
    expr_varref_t (const xpub3_ref_t &x);
    bool to_xdr (xpub3_expr_t *x) const;
    str to_identifier () const { return _name; }
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    const char *get_obj_name () const { return "pub3::expr_varref_t"; }
  protected:
    ptr<const pval_t> deref_step (eval_t *e) const;
    str _name;
  };

  //-----------------------------------------------------------------------

  class expr_str_t : public expr_static_t {
  public:
    expr_str_t (const str &s) : _val (s) {}
    expr_str_t (const xpub3_str_t &x);

    str to_str () const;
    bool to_bool () const;
    scalar_obj_t to_scalar () const;
    bool to_null () const;
    ptr<rxx> to_regex () const;

    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    const char *get_obj_name () const { return "pub3::expr_str_t"; }
    bool to_len (size_t *s) const;
    bool to_xdr (xpub3_expr_t *x) const;

    str eval_as_str (eval_t e) const;
    bool eval_as_null (eval_t e) const;
    ptr<rxx> eval_as_regex (eval_t e) const { return to_regex (); }

  protected:
    str _val;
  };

  //-----------------------------------------------------------------------

  class expr_number_t : public expr_static_t {
  public:
    expr_number_t () : expr_static_t () {}

    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    str to_str () const { return to_scalar ().to_str (); }

  };

  //-----------------------------------------------------------------------

  class expr_int_t : public expr_number_t {
  public:
    expr_int_t (int64_t i) : _val (i) {}
    expr_int_t (const xpub3_int_t &x);

    bool to_bool () const { return _val; }
    scalar_obj_t to_scalar () const;
    int64_t to_int () const { return _val; }
    u_int64_t to_uint () const;
    bool to_xdr (xpub3_expr_t *x) const;
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    const char *get_obj_name () const { return "pub3::expr_int_t"; }

    static ptr<expr_int_t> alloc (int64_t i) 
    { return New refcounted<expr_int_t> (i); }

  protected:
    int64_t _val;
  };

  //-----------------------------------------------------------------------

  class expr_uint_t : public expr_number_t {
  public:
    expr_uint_t (u_int64_t i) : _val (i) {}
    expr_uint_t (const xpub3_uint_t &x);

    bool to_bool () const { return _val; }
    u_int64_t to_uint () const { return _val; }
    int64_t to_int () const;

    scalar_obj_t to_scalar () const;

    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    bool to_xdr (xpub3_expr_t *x) const;
    const char *get_obj_name () const { return "pub3::expr_uint_t"; }

    static ptr<expr_uint_t> alloc (u_int64_t i) 
    { return New refcounted<expr_uint_t> (i); }
  private:
    u_int64_t _val;
  };

  //-----------------------------------------------------------------------

  class expr_double_t : public expr_number_t {
  public:
    expr_double_t (double d) : _val (d) {}
    expr_double_t (const xpub3_double_t &d);

    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    bool to_bool () const { return _val != 0; }
    double to_double () const { return _val; }
    scalar_obj_t to_scalar () const;

    bool to_xdr (xpub3_expr_t *x) const;
    const char *get_obj_name () const { return "pub3::expr_double_t"; }

    static ptr<expr_double_t> alloc (double i) 
    { return New refcounted<expr_double_t> (i); }
  private:
    double _val;
  };

  //-----------------------------------------------------------------------

  class expr_list_t : public expr_t, 
		      public vec<ptr<expr_t> > ,
		      public vec_iface_t {
  public:

    typedef vec<ptr<expr_t> > vec_base_t;

    expr_list_t (int lineno = -1) : expr_t (lineno) {}
    expr_list_t (const xpub3_expr_list_t &x);

    bool to_xdr (xpub3_expr_t *) const;
    bool to_xdr (xpub3_expr_list_t *) const;

    // vec_iface_t interface
    ptr<const pval_t> lookup (ssize_t i, bool *ib = NULL) const;
    ptr<pval_t> lookup (ssize_t i, bool *ib = NULL);
    ptr<vec_iface_t> to_vec_iface () { return mkref (this); }
    ptr<const vec_iface_t> to_vec_iface () const { return mkref (this); }
    void set (size_t i, ptr<pval_t> v);
    void push_back (ptr<pval_t> v);
    ptr<expr_t> &push_back () { return vec_base_t::push_back (); }
    size_t size () const { return vec_base_t::size (); }
    void setsize (size_t s) { vec_base_t::setsize (s); }
    bool to_len (size_t *s) const;
    bool to_bool () const { return size () > 0; }

    ptr<pval_t> eval_freeze (eval_t e) const;
    ptr<const pval_t> eval (eval_t e) const { return mkref (this); }
    ptr<const vec_iface_t> eval_as_vec () const { return mkref (this); }
    str eval_as_str (eval_t e) const;
    bool eval_as_bool (eval_t e) const { return to_bool (); }
    ptr<rxx> eval_as_regex (eval_t e) const;

    // to JSON-style string
    scalar_obj_t to_scalar () const;
    str to_str () const;

    static ptr<expr_list_t> alloc (int l) 
    { return New refcounted<expr_list_t> (l); }
    static ptr<expr_list_t> alloc (const xpub3_expr_list_t &x) 
    { return New refcounted<expr_list_t> (x); }
    static ptr<expr_list_t> alloc (const xpub3_expr_list_t *x);

    ptr<const expr_list_t> to_expr_list () const { return mkref (this); }
    ptr<expr_list_t> to_expr_list () { return mkref (this); }

    const char *get_obj_name () const { return "pub3::expr_list_t"; }
  };
  
  //-----------------------------------------------------------------------

  class rxx_factory_t {
  public:
    rxx_factory_t () {}
    static ptr<rxx> compile (str body, str opts, str *err);
  private:
    qhash<str, ptr<rxx> > _cache;
    ptr<rxx> _compile (str body, str opts, str *err);
  };

  //-----------------------------------------------------------------------


  class expr_regex_t : public expr_t {
  public:
    expr_regex_t (int lineno);
    expr_regex_t (const xpub3_regex_t &x);
    expr_regex_t (ptr<rxx> x, str b, str o, int lineno);
    const char *get_obj_name () const { return "pub3::expr_regex_t"; }
    bool to_xdr (xpub3_expr_t *x) const;

    ptr<const pval_t> eval (eval_t e) const { return mkref (this); }
    ptr<pval_t> eval_freeze (eval_t e) const;
    str eval_as_str (eval_t e) const { return _body; }
    ptr<rxx> eval_as_regex (eval_t e) const { return _rxx; }

    str to_str () const { return _body; }
    ptr<rxx> to_regex () const { return _rxx; }
    ptr<expr_regex_t> to_regex_obj () { return mkref (this); }
    
  private:
    ptr<rxx> _rxx;
    str _body, _opts;
  };


  //-----------------------------------------------------------------------

  class expr_shell_str_t : public expr_t {
  public:
    expr_shell_str_t (int lineno);
    expr_shell_str_t (const str &s, int lineno);
    expr_shell_str_t (ptr<expr_t> e, int lineno);
    expr_shell_str_t (const xpub3_shell_str_t &x);
    
    ptr<const pval_t> eval (eval_t e) const { return eval_freeze (e); }
    ptr<pval_t> eval_freeze (eval_t e) const;
    scalar_obj_t eval_as_scalar (eval_t e) const;
    str eval_as_str (eval_t e) const;
    ptr<rxx> eval_as_regex (eval_t e) const;

    ptr<expr_t> compact () const;
    void add (ptr<expr_t> e) { _els->push_back (e); }
    bool to_xdr (xpub3_expr_t *x) const;
    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    const char *get_obj_name () const { return "pub3::expr_shell_str_t"; }

  protected:
    ptr<expr_list_t> _els;
    str eval_internal (eval_t e) const;

  private:
    void make_str (strbuf *b, vec<str> *v);
    mutable str _cache;
  };

  //-----------------------------------------------------------------------

  class expr_dict_t : public expr_t {
  public:
    expr_dict_t (int lineno = -1) 
      : expr_t (lineno), _dict (New refcounted<aarr_arg_t> ()) {}
    expr_dict_t (const xpub3_dict_t &x);

    // To JSON-style string
    scalar_obj_t to_scalar () const;
    str to_str () const;

    ptr<const pval_t> eval (eval_t e) const { return mkref (this); }
    ptr<pval_t> eval_freeze (eval_t e) const;
    str eval_as_str (eval_t e) const;
    bool to_len (size_t *s) const;

    void add (nvpair_t *p);
    const char *get_obj_name () const { return "pub3::expr_dict_t"; }
    bool to_xdr (xpub3_expr_t *x) const;

    ptr<aarr_t> to_dict () { return _dict; }
    ptr<aarr_arg_t> dict () { return _dict; }
    ptr<aarr_arg_t> to_aarr () { return _dict; }
    ptr<const aarr_t> to_dict () const { return _dict; }
    ptr<const aarr_arg_t> dict () const { return _dict; }
    ptr<const aarr_arg_t> to_aarr () const { return _dict; }
    ptr<aarr_arg_t> copy_dict_stub () const { return _dict; }

    ptr<const expr_dict_t> to_expr_dict () const { return mkref (this); }
    ptr<expr_dict_t> to_expr_dict () { return mkref (this); }

    void dump2 (dumper_t *d) const { /* XXX implement me */ }
    size_t size () const { return _dict ? _dict->size () : 0; }

  protected:
    ptr<aarr_arg_t> _dict;
  }; 

  //-----------------------------------------------------------------------

  class inline_var_t : public pfile_el_t {
  public:
    inline_var_t (ptr<pub3::expr_t> e, int l) : _expr (e), _lineno (l) {}
    inline_var_t (const xpub3_inline_var_t &x);
    void output (output_t *o, penv_t *e) const;
    pfile_el_type_t get_type () const { return PFILE_PUB3_VAR; }
    const char *get_obj_name () const { return "pub3::inline_var_t"; }
    void dump2 (dumper_t *d) const;
    bool to_xdr (xpub_obj_t *x) const;
  private:
    const ptr<expr_t> _expr;
    const int _lineno;
  };

  //-----------------------------------------------------------------------
};

#endif /* _LIBPUB_PUB3EXPR_H_ */

