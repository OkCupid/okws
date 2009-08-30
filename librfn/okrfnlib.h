// -*-c++-*-
/* $Id: web.h 4029 2009-01-30 13:28:14Z max $ */

#ifndef _LIBRFN_OKRFNLIB_H_
#define _LIBRFN_OKRFNLIB_H_

#include "pub3.h"
#include "qhash.h"
#include "okrfn.h"

namespace rfn1 {

  using namespace pub3;

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

  class type_t : public scalar_fn_t {
  public:
    type_t (const str &n, ptr<expr_list_t> el, int lineno);
  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class join_t : public scalar_fn_t {
  public:
    join_t (const str &n, ptr<expr_list_t> el, int lineno);
  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _join_str;
    ptr<expr_t> _join_list;
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

  class append_t : public runtime_fn_t {
  public:
    append_t (const str &n, ptr<expr_list_t> el, int lineno);

    static ptr<runtime_fn_t>
    constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err);

    ptr<pval_t> eval_freeze (eval_t e) const;
    ptr<const pval_t> eval (eval_t e) const;

  private:
    ptr<expr_t> eval_internal (eval_t e) const;
    ptr<expr_t> _list;
  };

  //-----------------------------------------------------------------------

  class map_t : public runtime_fn_t {
  public:
    map_t (const str &n, ptr<expr_list_t> el, int lineno, ptr<expr_t> d, 
	   ptr<expr_t> l);

    static ptr<runtime_fn_t>
    constructor (const str &n, ptr<expr_list_t> e, int lieno, str *err);

    ptr<pval_t> eval_freeze (eval_t e) const;
    ptr<const pval_t> eval (eval_t e) const;

  private:
    ptr<expr_t> eval_internal (eval_t e) const;
    ptr<expr_t> eval_internal (eval_t e, ptr<const aarr_t> mp, 
			       ptr<const expr_t> arg) const;
    ptr<expr_t> _map;
    ptr<expr_t> _list;
  };

  //-----------------------------------------------------------------------

  class is_null_t : public predicate_t {
  public:
    is_null_t (const str &n, ptr<expr_list_t> l, int lineno, ptr<expr_t> e);

    static ptr<runtime_fn_t> 
    constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err);

  protected:
    bool eval_internal_bool (eval_t e) const;
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class rxx_fn_t : public predicate_t {
  public:
    rxx_fn_t (const str &n, ptr<expr_list_t> l, int lineno,
	      ptr<expr_t> rxx, ptr<expr_t> val, bool match);

    static ptr<runtime_fn_t> 
    constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err);

  protected:
    bool eval_internal_bool (eval_t e) const;

    ptr<expr_t> _rxx;
    ptr<expr_t> _val;
    bool _match;
  };

  //-----------------------------------------------------------------------

  class tolower_t : public scalar_fn_t {
  public:
    tolower_t (const str &n, ptr<expr_list_t> l, int lineno);

  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class toupper_t : public scalar_fn_t {
  public:
    toupper_t (const str &n, ptr<expr_list_t> l, int lineno);

  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class html_escape_t : public scalar_fn_t {
  public:
    html_escape_t (const str &n, ptr<expr_list_t> l, int lineno);

  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class tag_escape_t : public scalar_fn_t {
  public:
    tag_escape_t (const str &n, ptr<expr_list_t> l, int lineo);

  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg;
    ptr<expr_t> _ok_rxx;
  };

  //-----------------------------------------------------------------------

  class json_escape_t : public scalar_fn_t {
  public:
    json_escape_t (const str &n, ptr<expr_list_t> l, int lineno);

  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class hidden_escape_t : public scalar_fn_t {
  public:
    hidden_escape_t (const str &n, ptr<expr_list_t> l, int lineno);
  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class substring_t : public scalar_fn_t {
  public:
    substring_t (const str &n, ptr<expr_list_t> l, int lineno);

  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg;
    ptr<expr_t> _start, _len;
  };

  //-----------------------------------------------------------------------

  class default_t : public scalar_fn_t {
  public:
    default_t (const str &n, ptr<expr_list_t> l, int lineno);

  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg, _def_val;
  };

  //-----------------------------------------------------------------------

  class strip_t : public scalar_fn_t {
  public:
    strip_t (const str &n, ptr<expr_list_t> l, int lineno);

  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class url_escape_t : public scalar_fn_t {
  public:
    url_escape_t (const str &n, ptr<expr_list_t> l, int lineno);
  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class url_unescape_t : public scalar_fn_t {
  public:
    url_unescape_t (const str &n, ptr<expr_list_t> l, int lineno);
  private:
    scalar_obj_t eval_internal (eval_t e) const;
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class import_t : public runtime_fn_t {
  public:
    import_t (const str &n, ptr<expr_list_t> l, int lineno);

    ptr<const pval_t> eval (eval_t e) const;
    ptr<pval_t> eval_freeze (eval_t e) const;

    static ptr<runtime_fn_t>
    constructor (const str &nm, ptr<expr_list_t> e, int lineno, str *err);
  private:
    ptr<pval_t> eval_internal (eval_t e) const;
  };

  //-----------------------------------------------------------------------

  class version_str_t : public scalar_fn_t {
  public:
    version_str_t (const str &n, ptr<expr_list_t> l, int lineno)
      : scalar_fn_t (n, l, lineno) {}
  private:
    scalar_obj_t eval_internal (eval_t e) const;
  };

  //-----------------------------------------------------------------------

  class version_int_t : public scalar_fn_t {
  public:
    version_int_t (const str &n, ptr<expr_list_t> l, int lineno)
      : scalar_fn_t (n, l, lineno) {}
  private:
    scalar_obj_t eval_internal (eval_t e) const;
  };

  //-----------------------------------------------------------------------

  class dict_fn_t : public runtime_fn_t {
  public:
    dict_fn_t (const str &n, ptr<expr_list_t> l, int lineno, ptr<expr_t> arg)
      : runtime_fn_t (n, l, lineno), _arg (arg) {}

    template<class T> static ptr<runtime_fn_t>
    constructor (const str &nm, ptr<expr_list_t> e, int lineno, str *err)
    {
      ptr<runtime_fn_t> r;
      size_t narg = e ? e->size () : size_t (0);

      if (narg != 1) {
	*err = "values(),keys(),items() take 1 argument (a dict)";
      } else {
	r = New refcounted<T> (nm, e, lineno, (*e)[0]);
      }
      return r;
    }


    ptr<const pval_t> eval (eval_t e) const { return eval_internal (e); }
    ptr<pval_t> eval_freeze (eval_t e) const { return eval_internal (e); }
  protected:
    virtual ptr<pval_t> eval_internal (eval_t e) const = 0;
    const nvtab_t *eval_to_nvtab (eval_t e, const char *fn) const;
    ptr<expr_t> eval_pval (eval_t e, const pval_t *in) const;
  private:
    ptr<expr_t> _arg;
  };

  //-----------------------------------------------------------------------

  class values_t : public dict_fn_t {
  public:
    values_t (const str &n, ptr<expr_list_t> l, int lineno, ptr<expr_t> arg)
      : dict_fn_t (n, l, lineno, arg) {}
  protected:
    ptr<pval_t> eval_internal (eval_t e) const;
  };

  //-----------------------------------------------------------------------

  class keys_t : public dict_fn_t {
  public:
    keys_t (const str &n, ptr<expr_list_t> l, int lineno, ptr<expr_t> arg)
      : dict_fn_t (n, l, lineno, arg) {}
  protected:
    ptr<pval_t> eval_internal (eval_t e) const;

  };

  //-----------------------------------------------------------------------

  class items_t : public dict_fn_t {
  public:
    items_t (const str &n, ptr<expr_list_t> l, int lineno, ptr<expr_t> arg)
      : dict_fn_t (n, l, lineno, arg) {}
  protected:
    ptr<pval_t> eval_internal (eval_t e) const;
  };

  //-----------------------------------------------------------------------

};

#endif /* _LIBRFN_OKRFNLIB_H_ */
