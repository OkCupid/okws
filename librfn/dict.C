// -*-c++-*-

#include "okrfn-int.h"
#include "okformat.h"
#include "pub3parse.h"
#include "okcgi.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  values_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
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

  const str values_t::DOCUMENTATION = "Returns a list of keys in a dictionary";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  keys_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
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

  const str keys_t::DOCUMENTATION = "Returns a list of keys in a dictionary";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  items_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
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

 const str items_t::DOCUMENTATION =
    "Return the list of items in the dictionary. Each "
    "item is represented as a list of 2 elemenents, the first being the "
    "key and the second being the value.";

 //-----------------------------------------------------------------------

  ptr<const expr_t>
  remove_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_dict_t> d = args[0]._d;
    const str key = args[1]._s;
    bool found = d->lookup(key);
    d->remove(key);
    return expr_bool_t::alloc(found);
  }

  //-----------------------------------------------------------------------
  
  ptr<const expr_t>
  json2pub_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
      str s = args[0]._s;
      bool verbose = false;
      if (args.size() > 1) verbose = args[1]._b;

      if (verbose) {
          ptr<json_parser_t> parser = New refcounted<json_parser_t> ();
          ptr<expr_t> ret = parser->mparse (s);
          if (!ret || ret->is_null()) {
              str err = str("Invalid JSON: ") << s << "\n";
              for (auto& e: parser->get_errors())
                  err = err << e << "\n";
              report_error (p, err);
          }
          return ret;
      }
      else {
          ptr<expr_t> ret;
          ret = json_parser_t::parse (s);
          return ret;
      }
  }

  //-----------------------------------------------------------------------

  const str json2pub_t::DOCUMENTATION =
    "Converts a string into a JSON-encoded object";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  cgi2pub_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
      using namespace pub3;
      str s = args[0]._s;
      ptr<cgi_t> cgi = cgi_t::str_parse(s);
      pub3::obj_t ret;
      for (auto p = cgi->first(); p; p = cgi->next(p))
          ret(p->key) = p->vals;
      return ret.dict();
  }

  //-----------------------------------------------------------------------

  const str cgi2pub_t::DOCUMENTATION =
    "Converts a string into a CGI dictionary";

  //-----------------------------------------------------------------------

};
