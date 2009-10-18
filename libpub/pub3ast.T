// -*-c++-*-

#include "pub3ast.h"
#include "pub3parse.h"

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

  zone_html_t::zone_html_t (location_t l) : 
    zone_container_t (l),
    _preserve_white_space (false)
  {
    // reserve one free slot!
    _children.push_back (NULL);
  }

  //-----------------------------------------------------------------------

  ptr<zone_html_t>
  zone_html_t::alloc (ptr<zone_t> z)
  {
    ptr<zone_html_t> ret;
    if (z) { ret = z->zone_html (); }
    if (!ret) { 
      ret = New refcounted<zone_html_t> (location ()); 
      if (z) { ret->add (z); }
    }
    return ret;
  }

  //-----------------------------------------------------------------------
  
  void
  zone_html_t::add (ptr<zone_t> z)
  {
    _children.push_back (z);
  }
  
  //-----------------------------------------------------------------------

  ptr<zone_text_t>
  zone_html_t::push_zone_text ()
  {
    ptr<zone_text_t> x;


    if (!_children.size ()  || 
	!_children.back ()  || 
	!(x = _children.back ()->zone_text ())) {

      x = zone_text_t::alloc ();
      _children.push_back (x);

    }
    return x;
  }

  //-----------------------------------------------------------------------

  void
  zone_html_t::add (str s)
  {
    ptr<zone_text_t> x = push_zone_text ();
    x->add (s);
  }

  //-----------------------------------------------------------------------

  void
  zone_html_t::add (char c)
  {
    ptr<zone_text_t> x = push_zone_text ();
    x->add (c);
  }

  //-----------------------------------------------------------------------

  ptr<zone_html_t> zone_html_t::alloc ()
  { return New refcounted<zone_html_t> (location ()); }

  //-----------------------------------------------------------------------

  ptr<zone_text_t> zone_text_t::alloc ()
  { return New refcounted<zone_text_t> (location ()); }

  //-----------------------------------------------------------------------

  ptr<zone_text_t> zone_text_t::alloc (char c)
  { 
    ptr<zone_text_t> z = New refcounted<zone_text_t> (location ()); 
    z->add (c);
    return z;
  }

  //-----------------------------------------------------------------------

  ptr<zone_text_t> zone_text_t::alloc (str s)
  { 
    ptr<zone_text_t> z = New refcounted<zone_text_t> (location ()); 
    z->add (s);
    return z;
  }

  //-----------------------------------------------------------------------

  void
  zone_text_t::add (str s)
  {
    _hold.push_back (s);
    _b << s;
  }

  //-----------------------------------------------------------------------

  void
  zone_text_t::add (char c)
  {
    _b.fmt ("%c", c);
  }

  //-----------------------------------------------------------------------

  ptr<zone_inline_expr_t> zone_inline_expr_t::alloc (ptr<expr_t> x)
  { return New refcounted<zone_inline_expr_t> (location (), x); }

  //-----------------------------------------------------------------------

  ptr<zone_pub_t> zone_pub_t::alloc ()
  { return New refcounted<zone_pub_t> (location ()); }

  //-----------------------------------------------------------------------

  void zone_pub_t::reserve () { _statements.push_back (NULL); }
  void zone_pub_t::add (ptr<statement_t> s) { _statements.push_back (s); }

  //-----------------------------------------------------------------------

  void zone_pub_t::unreserve ()
  {
    if (!_statements.front ()) _statements.pop_front ();
  }

  //-----------------------------------------------------------------------

  void
  zone_pub_t::add (zone_pub_t::pair_t p)
  {
    if (p.first)  add (p.first);
    if (p.second) add (p.second);
  }

  //-----------------------------------------------------------------------

  void
  zone_pub_t::take_reserved_slot (ptr<statement_t> s)
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
    : statement_t (l), _zone (z) {}
  
  //-----------------------------------------------------------------------

  bool
  for_t::add_params (ptr<expr_list_t> l)
  {

    bool ret = true;
    if (!l || l->size () < 1 || l->size () > 2) {
      parse_error ("for: takes 2 arguments (simple identifier and array)\n");
      ret = false;
    } else if (!(_iter = (*l)[0]->to_identifier ()) || !_iter.len ()) {
      parse_error ("for: argument 1 must be an identifier\n");
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
  if_t::add_clauses (ptr<if_clause_list_t> ccl)
  {
    if (!_clauses) { _clauses = ccl; } 
    else if (ccl) { *_clauses += *ccl; }
  }

  //-----------------------------------------------------------------------
  
  void
  if_t::add_clause (ptr<if_clause_t> cc)
  {
    if (cc) {
      if (!_clauses) {
	_clauses = New refcounted<if_clause_list_t> ();
      }
      _clauses->push_back (cc);
    }
  }
  
  //-----------------------------------------------------------------------

  ptr<locals_t> locals_t::alloc () 
  { return New refcounted<locals_t> (location ()); }

  //-----------------------------------------------------------------------

  ptr<universals_t> universals_t::alloc () 
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
  { return New refcounted<case_list_t> (); }
  ptr<switch_t> switch_t::alloc () 
  { return New refcounted<switch_t> (location ()); }

  //-----------------------------------------------------------------------

  ptr<include_t> include_t::alloc () 
  { return New refcounted<include_t> (location ()); }
  ptr<load_t> load_t::alloc () { return New refcounted<load_t> (location ()); }

  //-----------------------------------------------------------------------

  tamed void
  include_t::publish (publish_t p, status_ev_t ev)
  {
    tvars {
      xpub_status_t rs (XPUB_STATUS_OK);
      str fn;
      ptr<const expr_dict_t> cd;
    }
    
    if (!_file || !(fn = _file->eval_as_str (p)) || fn.len () == 0) {
      str e = "include: cannot evaluate filename";
      p.report_error (e, _location);
    } else if (_dict && !(cd = _dict->eval_as_dict (p))) {
      str e = "include: cannot evalute parameters";
      p.report_error (e, _location);
    } else {
      twait { p.publish (fn, cow_bindtab_t::alloc (cd), mkevent (rs)); }
    }
    ev->trigger (rs);
  }
 

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
      str f = fnname ();
      err = strbuf ("%s take 1 or 2 arguments; a filename and an optional "
		    "binding list", f.cstr ());
      if (errp) *errp = err;
      ret = false;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  fndef_t::add_params (ptr<identifier_list_t> l)
  {
    if (!_params) {
      _params = l;
    } else if (l) { 
      *_params += *l;
    }
  }

  //-----------------------------------------------------------------------

  void
  fndef_t::add_body (ptr<zone_t> z) 
  {
    _body = z;
  }

  //-----------------------------------------------------------------------

  ptr<print_t> print_t::alloc () 
  { return New refcounted<print_t> (plineno ()); }

  //-----------------------------------------------------------------------

};

