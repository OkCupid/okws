// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */


#ifndef _LIBPUB_PUB3DATA_H_
#define _LIBPUB_PUB3DATA_H_

#include "pub.h"
#include "parr.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class eval_t {
  public:
    eval_t (penv_t *e, output_t *o) : _env (e), _output (o), _loud (false) {}

    penv_t *penv () const { return _env; }
    output_t *output () const { return _output; }

    bool set_loud (bool b);
    bool loud () const { return _loud; }

  private:
    penv_t *_env;
    output_t *_output;
    bool _loud;
  };

  //-----------------------------------------------------------------------

  class expr_t {
  public:
    expr_t (int lineno = -1) : _lineno (lineno), _cached (false) {}
    virtual ~expr_t () {}

    virtual bool to_xdr (xpub3_expr_t *x) const = 0;

    static ptr<expr_t> alloc (const xpub3_expr_t &x);
    static ptr<expr_t> alloc (const xpub3_expr_t *x);
    static ptr<vec<ptr<expr_t> > > alloc (const xpub3_expr_list_t &x);
    static ptr<vec<ptr<expr_t> > > alloc (const xpub3_expr_list_t *x);

    static void expr_to_xdr (ptr<expr_t> e, rpc_ptr<xpub3_expr_t> *x);

    static void expr_to_xdr (const ptr<vec<ptr<expr_t> > > in, 
			     xpub3_expr_list_t *out);
    
    virtual bool eval_as_bool (eval_t *e) const;
    virtual int64_t eval_as_int (eval_t *e) const;
    virtual u_int64_t eval_as_uint (eval_t *e) const;
    virtual str eval_as_str (eval_t *e) const;
    virtual scalar_obj_t eval_as_scalar (eval_t *e) const;
    virtual bool is_null (eval_t *e) const;
    virtual ptr<const aarr_t> eval_as_dict (eval_t *e) const;
    virtual ptr<const parr_mixed_t> eval_as_vec (eval_t *e) const;

  protected:
    void report_error (eval_t *e, str n) const;
    virtual ptr<const pval_t> eval_as_pval (eval_t *e) const { return NULL; }

    int _lineno;

    mutable bool _cached;
    mutable scalar_obj_t _so;
  };
  
  //-----------------------------------------------------------------------

  class expr_logical_t : public expr_t {
  public:
    expr_logical_t (int l = -1) : expr_t (l) {}
    int64_t eval_as_int (eval_t *e) const { return eval_as_bool (e); }
    u_int64_t eval_as_uint (eval_t *e) const { return eval_as_bool (e); }
    scalar_obj_t eval_as_scalar (eval_t *e) const;
    str eval_as_str (eval_t *e) const;
    bool is_null (eval_t *e) const { return false; }
  };

  //-----------------------------------------------------------------------

  class expr_OR_t : public expr_logical_t {
  public:
    expr_OR_t (ptr<expr_t> t1, ptr<expr_t> t2) : _t1 (t1), _t2 (t2) {}
    expr_OR_t (const xpub3_or_t &x);
    bool to_xdr (xpub3_expr_t *x) const;
    bool eval_as_bool (eval_t *e) const;
    ptr<expr_t> _t1, _t2;
  };

  //-----------------------------------------------------------------------

  class expr_AND_t : public expr_logical_t  {
  public:
    expr_AND_t (ptr<expr_t> f1, ptr<expr_t> f2) : _f1 (f1), _f2 (f2) {}
    expr_AND_t (const xpub3_and_t &x);
    bool eval_as_bool (eval_t *e) const;
    bool to_xdr (xpub3_expr_t *x) const;
  protected:
    ptr<expr_t> _f1, _f2;
  };

  //-----------------------------------------------------------------------

  class expr_NOT_t : public expr_logical_t  {
  public:
    expr_NOT_t (ptr<expr_t> e) : _e (e) {}
    expr_NOT_t (const xpub3_not_t &x);
    bool eval_as_bool (eval_t *e) const;
    bool to_xdr (xpub3_expr_t *x) const;
  protected:
    ptr<expr_t> _e;
  };

  //-----------------------------------------------------------------------

  class expr_relation_t : public expr_logical_t {
  public:
    expr_relation_t (ptr<expr_t> l, ptr<expr_t> r, xpub3_relop_t op, int lineno)
      : expr_logical_t (lineno), _l (l), _r (r), _op (op) {}
    expr_relation_t (const xpub3_relation_t &x);
    bool eval_as_bool (eval_t *e) const;
    bool to_xdr (xpub3_expr_t *x) const;

  protected:
    ptr<expr_t> _l, _r;
    xpub3_relop_t _op;
  };

  //-----------------------------------------------------------------------

  class expr_arithmetic_t : public expr_t {
  public:
    expr_arithmetic_t (int l) : expr_t (l) {}

    bool eval_as_bool (eval_t *e) const;
    str eval_as_str (eval_t *e) const;
    int64_t eval_as_int (eval_t *e) const;
    u_int64_t eval_as_uint (eval_t *e) const;
    ptr<const pval_t> eval_as_pval (eval_t *e) const;
    scalar_obj_t eval_as_scalar (eval_t *e) const;

  protected:
    virtual scalar_obj_t eval_as_scalar_nocache (eval_t *e) const = 0;
  };

  //-----------------------------------------------------------------------

  class expr_add_t : public expr_arithmetic_t {
  public:
    expr_add_t (ptr<expr_t> t1, ptr<expr_t> t2, bool pos, int lineno)
      : expr_arithmetic_t (lineno), _t1 (t1), _t2 (t2), _pos (pos) {}
    expr_add_t (const xpub3_add_t &x);

    bool to_xdr (xpub3_expr_t *x) const;
    scalar_obj_t eval_as_scalar_nocache (eval_t *e) const;
  protected:
    ptr<expr_t> _t1, _t2;
    bool _pos;
  };

  //-----------------------------------------------------------------------

  class expr_EQ_t : public expr_logical_t {
  public:
    expr_EQ_t (ptr<expr_t> o1, ptr<expr_t> o2, bool pos, int ln) 
      : expr_logical_t (ln), _o1 (o1), _o2 (o2), _pos (pos) {}
    expr_EQ_t (const xpub3_eq_t &x);

    bool eval_as_bool (eval_t *e) const;
    bool to_xdr (xpub3_expr_t *x) const;
  protected:
    ptr<expr_t> _o1, _o2;
    bool _pos;
  };

  //-----------------------------------------------------------------------

  class expr_dictref_t : public expr_t {
  public:
    expr_dictref_t (ptr<expr_t> d, const str &k, int lineno)
      : expr_t (lineno), _dict (d), _key (k) {}
    expr_dictref_t (const xpub3_dictref_t &x);
    bool to_xdr (xpub3_expr_t *x) const;
  protected:
    ptr<const pval_t> eval_as_pval (eval_t *e) const;
    ptr<expr_t> _dict;
    str _key;
  };

  //-----------------------------------------------------------------------

  class expr_vecref_t : public expr_t {
  public:
    expr_vecref_t (ptr<expr_t> v, ptr<expr_t> i, int l) 
      : expr_t (l), _vec (v), _index (i) {}
    expr_vecref_t (const xpub3_vecref_t &x);

    bool to_xdr (xpub3_expr_t *x) const;
  protected:
    ptr<const pval_t> eval_as_pval (eval_t *e) const;
    ptr<expr_t> _vec;
    ptr<expr_t> _index;
  };

  //-----------------------------------------------------------------------

  class expr_ref_t : public expr_t {
  public:
    expr_ref_t (const str &s, int l) : expr_t (l), _name (s) {}
    expr_ref_t (const xpub3_ref_t &x);
    bool to_xdr (xpub3_expr_t *x) const;
  protected:
    ptr<const pval_t> eval_as_pval (eval_t *e) const;
    str _name;
  };

  //-----------------------------------------------------------------------

  class expr_str_t : public expr_t {
  public:
    expr_str_t (const str &s) : _val (s) {}
    expr_str_t (const xpub3_str_t &x);

    bool eval_as_bool (eval_t *e) const;
    str eval_as_str (eval_t *e) const;
    scalar_obj_t eval_as_scalar (eval_t *e) const;
    int64_t eval_as_int (eval_t *e) const;
    u_int64_t eval_as_uint (eval_t *e) const;
    bool is_null (eval_t *e) const;
    ptr<const aarr_t> eval_as_dict (eval_t *e) const { return NULL; }
    ptr<const parr_mixed_t> eval_as_vec (eval_t *e) const { return NULL; }

    bool to_xdr (xpub3_expr_t *x) const;
  protected:
    ptr<const pval_t> eval_as_pval (eval_t *e) const;
    str _val;
  };

  //-----------------------------------------------------------------------

  class expr_number_t : public expr_t {
  public:
    expr_number_t () : expr_t () {}

    bool is_null (eval_t *e) const { return false; }
    ptr<const aarr_t> eval_as_dict (eval_t *e) const { return NULL; }
    ptr<const parr_mixed_t> eval_as_vec (eval_t *e) const { return NULL; }
  protected:
    ptr<const pval_t> eval_as_pval (eval_t *e) const;
  };
  

  //-----------------------------------------------------------------------

  class expr_int_t : public expr_number_t {
  public:
    expr_int_t (int64_t i) : _val (i) {}
    expr_int_t (const xpub3_int_t &x);

    bool eval_as_bool (eval_t *e) const { return _val; }
    scalar_obj_t eval_as_scalar (eval_t *e) const;
    bool to_xdr (xpub3_expr_t *x) const;

  protected:
    int64_t _val;
  };

  //-----------------------------------------------------------------------

  class expr_uint_t : public expr_number_t {
  public:
    expr_uint_t (u_int64_t i) : _val (i) {}
    expr_uint_t (const xpub3_uint_t &x);

    bool eval_as_bool (eval_t *e) const { return _val; }
    scalar_obj_t eval_as_scalar (eval_t *e) const;
    bool to_xdr (xpub3_expr_t *x) const;
  private:
    u_int64_t _val;
  };

  //-----------------------------------------------------------------------

  class expr_double_t : public expr_number_t {
  public:
    expr_double_t (double d) : _val (d) {}
    expr_double_t (const xpub3_double_t &d);

    bool eval_as_bool (eval_t *e) const { return _val != 0; }
    scalar_obj_t eval_as_scalar (eval_t *e) const;
    bool to_xdr (xpub3_expr_t *x) const;
  private:
    double _val;
  };

  //-----------------------------------------------------------------------

  typedef vec<ptr<expr_t> > expr_list_t;

  //-----------------------------------------------------------------------

};

#endif /* _LIBPUB_PUB3EXPR_H_ */

