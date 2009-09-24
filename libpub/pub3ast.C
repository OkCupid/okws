#include "pub3zone.h"


namespace pub3 {

  //-----------------------------------------------------------------------

  static const location_t &location () 
  { return parser_t::current ()->location (); }

  //-----------------------------------------------------------------------

  bool
  zone_text_t::add (ptr<zone_t> z)
  {
    bool ret = true;
    str s = z->to_str ();

    if (!s) {
      ret = false;
    } else if (s.len ()) {
      _hold.push_back (s);
      _b << s;
    }

    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  zone_html_t::add (ptr<zone_t> z)
  {
    zone_html_t *zh;

    if ((zh = z->zone_html ()) && 
	zh->preserve_white_space () == preserve_white_space ()) {
      _children += *zh->children ();
    } else if (!_children.size () || !_children.back ().add (z)) {
      _childen.push_back (z);
    }
    return true;
  }
  
  //-----------------------------------------------------------------------

  ptr<zone_html_t> zone_html_t::alloc (int pws)
  { return New refcounted<zone_html_t> (location (), pws) }

  //-----------------------------------------------------------------------

  ptr<zone_text_t> zone_text_t::alloc ()
  { return New refcounted<zone_text_t> (location ()); }

  //-----------------------------------------------------------------------

  ptr<zone_inline_expr_t> zone_inline_expr_t::alloc (ptr<expr_t> x)
  { return New refcounted<zone_inline_expr_t> (location (), x); }

  //-----------------------------------------------------------------------

  ptr<zone_pub_t> zone_pub_t::alloc ()
  { return New refcounted<zone_pub_t> (location ()); }

  //-----------------------------------------------------------------------

  void zone_pub_t::reserve () { _statements.push_back (NULL); }
  void zone_pub_t::add (ptr<expr_statemenet_t> s) { _statements.push_back (s); }

  //-----------------------------------------------------------------------

  void zone_pub_t::unreserve ()
  {
    if (!_statements.front ()) _statement.pop_front ();
  }

  //-----------------------------------------------------------------------

  void
  zone_pub_t::add (zone_pair_t p)
  {
    if (p.first)  add (p.first);
    if (p.second) add (p.second);
  }

  //-----------------------------------------------------------------------

  void
  zone_pub_t:::take_reserved_slot (ptr<expr_statement_t> s)
  {
    assert (_statements.size ());
    assert (!_statements[0]);
    if (s) { _statements[0] = s; }
    else { _statements.pop_front (); }
  }

  //-----------------------------------------------------------------------
  
  zone_pub_t::zone_pub_t (location_t l) : zone_t (l) { reserve (); }

  //-----------------------------------------------------------------------

  bool
  zone_pub_t::add (ptr<zone_t> z)
  {
    zone_pub_t *zp;
    bool ret = false;
    if ((zp = z->zone_pub ())) {
      ret = true;
      _statements += *zp->statements ();
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<statement_zone_t> 
  statement_zone_t::alloc (ptr<zone_t> z)
  {
    return New refcounted<statement_zone_t> (location (), z);
  }

  //-----------------------------------------------------------------------

  statement_zone_t::statement_zone_t (location_t l, ptr<zone_t> z)
    : _location (l), _zone (z) {}
  
  //-----------------------------------------------------------------------

  bool
  for_t::add_params (ptr<expr_list_t> l)
  {

    bool ret = true;
    if (!l || l->size () < 1 || l->size () > 2) {
      PWARN ("for: takes 2 arguments (simple identifier and array)\n");
      ret = false;
    } else if (!(_iter = (*l)[0]->to_identifier ()) || !_iter.len ()) {
      PWARN ("for: argument 1 must be an identifier\n");
      ret = false;
    } else if (l->size () > 1) {
      _arr = (*l)[1];
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool for_t::add_body (ptr<zone_t> z) { _body = z; return true; }
  bool for_t::add_empty (ptr<zone_t> z) { _empty = z; return true; }

  //-----------------------------------------------------------------------

  ptr<if_clause_t> if_clause_t::alloc () 
  { return New refcounted<if_clause_t> (location ()); }

  //-----------------------------------------------------------------------

  ptr<if_t> if_t::alloc () { return New refcounted<if_t> (location ()); }

  //-----------------------------------------------------------------------

  void
  if_t::add_clauses (ptr<cond_clause_list_t> ccl)
  {
    if (!_clauses) { _clauses = ccl; } 
    else if (ccl) { *_clauses += *ccl; }
  }

  //-----------------------------------------------------------------------
  
  void
  if_t::add_clause (ptr<cond_clause_t> cc)
  {
    if (cc) {
      if (!_clauses) {
	_clauses = New refcounted<cond_clause_list_t> ();
      }
      _clauses->push_back (cc);
    }
  }
  
  //-----------------------------------------------------------------------

  ptr<locals_t> locals_t::alloc () 
  { return New refcounted<locals_t> (location ()); }

  //-----------------------------------------------------------------------

  ptr<universals_t> univerals_t::alloc () 
  { return New refcounted<universals_t> (location ()); }

  //-----------------------------------------------------------------------

  void case_list_t::add_case (ptr<case_t> c) { if (c) { push_back (c); } }
  void switch_t::add_cases (ptr<case_list_t> l) { _cases = l; }
  void switch_t::add_key (ptr<expr_t> x) { _key = x; }
  void case_t::add_key (const str &k) { _key = k; }
  void case_t::add_zone (ptr<zone_t> z) { _zone = z; }

  //-----------------------------------------------------------------------

  ptr<case_t> case_t::alloc () { return New refcounted<case_t> (location ()); }
  ptr<case_list_t> case_list_t::alloc ()
  { return New refcounted<case_list_t> (location ()); }
  ptr<switch_t> switch_t::alloc () 
  { return New refcounted<switch_t> (location ()); }

  //-----------------------------------------------------------------------

  ptr<include_t> include_t::alloc () 
  { return New refcounted<include_t> (location ()); }
  ptr<load_t> load_t::alloc () { return New refcounted<load_t> (location ()); }

  //-----------------------------------------------------------------------

  bool
  include_t::add_args (ptr<expr_list_t> l, str *errp)
  {
    bool ret = true;
    str err;
    if (l && (l->size () >= 1 && l->size () <= 2)) {
      _file = (*l)[0];
      if (l->size () > 2) 
	_dict = (*l)[1];
    } else {
      str f = fnname ()
      err = strbuf ("%s take 1 or 2 arguments; a filename and an optional "
		    "binding list", f.cstr ());
      if (errp) *errp = err;
      ret = false;
    }
    return ret;
  }

  //-----------------------------------------------------------------------
};

