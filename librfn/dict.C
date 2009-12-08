// -*-c++-*-

#include "okrfn.h"
#include "okformat.h"
#include "pub3parse.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  values_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> l = New refcounted<expr_list_t> ();
    ptr<const expr_dict_t> d = args[0]._d;
    bindtab_t::const_iterator_t it (*d);
    const str *key;
    ptr<expr_t> cval;
    while ((key = it.next (&cval))) {
      ptr<expr_t> val;
      if (cval) val = cval->copy ();
      if (!val) val = expr_null_t::alloc ();
      l->push_back (val);
    }
    return l;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  keys_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> l = New refcounted<expr_list_t> ();
    ptr<const expr_dict_t> d = args[0]._d;
    bindtab_t::const_iterator_t it (*d);
    const str *key;
    while ((key = it.next ())) {
      l->push_back (expr_str_t::alloc (*key));
    }
    return l;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  items_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> l = New refcounted<expr_list_t> ();
    ptr<const expr_dict_t> d = args[0]._d;
    bindtab_t::const_iterator_t it (*d);
    const str *key;
    ptr<expr_t> cval;
    while ((key = it.next (&cval))) {
      ptr<expr_t> val;
      if (cval) val = cval->copy ();
      if (!val) val = expr_null_t::alloc ();
      ptr<expr_list_t> entry = expr_list_t::alloc ();
      entry->push_back (expr_str_t::alloc (*key));
      entry->push_back (val);
      l->push_back (entry);
    }
    return l;
  }

  //-----------------------------------------------------------------------
  
  ptr<const expr_t>
  json2pub_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    ptr<expr_t> ret;
    ret = json_parser_t::parse (s);
    return ret;
  }

  //-----------------------------------------------------------------------

};
