// -*-c++-*-
/* $Id: web.h 4029 2009-01-30 13:28:14Z max $ */

#ifndef _LIBRFN_OKRFN_H_
#define _LIBRFN_OKRFN_H_

#include "pub3.h"
#include "qhash.h"

namespace rfn1 {

  using namespace pub3;

  //-----------------------------------------------------------------------

  typedef ptr<runtime_fn_t> (*constructor_t) 
  (const str &n, ptr<expr_list_t> l, int line, str *err);

  //-----------------------------------------------------------------------

  class std_factory_t : public rfn_factory_t {
  public:

    std_factory_t ();

    ptr<runtime_fn_t>
    alloc (const str &s, ptr<expr_list_t> l, int lineno);

    qhash<str, constructor_t> _tab;

  };

  //-----------------------------------------------------------------------

  class scalar_fn_t : public runtime_fn_t {
  public:
    scalar_fn_t (const str &n, ptr<expr_list_t> el, int lineno)
      : runtime_fn_t (n, el, lineno) {}

    ptr<const pval_t> eval (eval_t e) const;
    ptr<pval_t> eval_freeze (eval_t e) const;
    scalar_obj_t eval_as_scalar (eval_t e) const { return eval_internal (e); }

    u_int64_t eval_as_uint (eval_t e) const 
    { return eval_internal (e).to_uint64 (); }

    int64_t eval_as_int (eval_t e) const 
    { return eval_internal (e).to_int (); }

  protected:
    virtual scalar_obj_t eval_internal (eval_t e) const = 0;

  };

  //-----------------------------------------------------------------------

  class random_t : public scalar_fn_t {
  public:
    
    random_t (const str &n, ptr<expr_list_t> el, int lineno, 
	      ptr<expr_t> l, ptr<expr_t> h);

    static ptr<runtime_fn_t> 
    constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err);

  private:
    scalar_obj_t eval_internal (eval_t e) const;

    ptr<expr_t> _low;
    ptr<expr_t> _high;
  };

  //-----------------------------------------------------------------------

  class len_t : public scalar_fn_t {
  public:
    len_t (const str &n, ptr<expr_list_t> el, int lineno, ptr<expr_t> l);

    static ptr<runtime_fn_t> 
    constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err);

  private:
    scalar_obj_t eval_internal (eval_t e) const;

    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class range_t : public runtime_fn_t {
  public:
    range_t (const str &n, ptr<expr_list_t> el, int lineno, 
	     ptr<expr_t> l, ptr<expr_t> h, ptr<expr_t> s);

    static ptr<runtime_fn_t> 
    constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err);

    ptr<pval_t> eval_freeze (eval_t e) const;
    ptr<const pval_t> eval (eval_t e) const;

  private:
    ptr<expr_list_t> eval_internal (eval_t e) const;

    ptr<expr_t> _l, _h, _s;
  };

  //-----------------------------------------------------------------------

  class is_null_t : public scalar_fn_t {
  public:
    is_null_t (const str &n, ptr<expr_list_t> l, int lineno, ptr<expr_t> e);

    static ptr<runtime_fn_t> 
    constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err);
    bool eval_as_bool (eval_t e) const { return eval_internal_bool (e); }

  private:
    scalar_obj_t eval_internal (eval_t e) const;
    bool eval_internal_bool (eval_t e) const;
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------


};

#endif /* _LIBRFN_OKRFN_H_ */
