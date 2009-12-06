// -*-c++-*-
/* $Id: web.h 4029 2009-01-30 13:28:14Z max $ */

#ifndef _LIBRFN_OKRFN_H_
#define _LIBRFN_OKRFN_H_

#include "pub3.h"
#include "qhash.h"

namespace rfn2 {

  using namespace pub3;

  //-----------------------------------------------------------------------

  const char *version_str ();
  const char *okws_version_str ();
  u_int64_t version_int ();
  u_int64_t okws_version_int ();

  //-----------------------------------------------------------------------

  class runtime_fn_t : public expr_t, public callable_t {
  public:
    runtime_fn_t (str n, str lib);
    bool might_block () const { return false; }

    virtual ptr<const expr_t> eval_to_val (publish_t *p, args_t args) const = 0;
    virtual void pub_to_val (publish_t *p, args_t args, cxev_t, CLOSURE) const;

  protected:
    ptr<const expr_t> eval_to_val (eval_t *e) const;
    ptr<mref_t> eval_to_ref (eval_t *e) const;
    void pub_to_val (publish_t *p, args_t args, cxev_t, CLOSURE) const;
    ptr<const callable_t> to_callable () const { return mkref (this); }
    ptr<runtime_fn_t> copy (eval_t *e) const;
    const char *get_obj_name () const { return "rfn1::runtime_fn_t"; }
  private:
    str n, str v;
  };

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

    virtual str eval_as_str (eval_t e) const
    { return eval_internal (e).to_str (); }

    template<class T, size_t x, size_t y> static ptr<runtime_fn_t>
    constructor (const str &nm, ptr<expr_list_t> e, int lineno, str *err)
    {
      ptr<runtime_fn_t> r;
      size_t narg = e ? e->size () : size_t (0);

      if (narg < x || narg > y) {
	if (y == x) {
	  *err = strbuf ("%s() function takes %zu arg%s; %zu given!\n", 
			 nm.cstr (), x, x == 1 ? "" : "s", narg);
	} else {
	  *err = strbuf ("%s() function takes %zu to %zu args; %zu given!\n",
			 nm.cstr(), x, y, narg);
	}
      } else {
	r = New refcounted<T> (nm, e, lineno);
      }
      return r;
    }

  protected:
    virtual scalar_obj_t eval_internal (eval_t e) const = 0;
  };

  //-----------------------------------------------------------------------

  class predicate_t : public scalar_fn_t {
  public:
    predicate_t (const str &n, ptr<expr_list_t> el, int lineno)
      : scalar_fn_t (n, el, lineno) {}
    bool eval_as_bool (eval_t e) const { return eval_internal_bool (e); }
    str eval_as_str (eval_t e) const;
  protected:
    scalar_obj_t eval_internal (eval_t e) const;
    virtual bool eval_internal_bool (eval_t e) const = 0;
  };

  //-----------------------------------------------------------------------
};

#endif /* _LIBRFN_OKRFN_H_ */
