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
  include_t::publish (publish_t p, status_ev_t ev) const
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
      twait { 
	p.publish (fn, _location, cow_bindtab_t::alloc (cd), mkevent (rs)); 
      }
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

  ptr<print_t> print_t::alloc () 
  { return New refcounted<print_t> (plineno ()); }

  //-----------------------------------------------------------------------

  ptr<expr_list_t>
  for_t::eval_list (publish_t p) const
  {
    bool old_silent = p.set_silent (true);
    ptr<mref_t> m = _arr->eval_to_ref (p);
    ptr<expr_t> x;
    if (m) x = m->get_value ();
    ptr<expr_list_t> ret;
    if (x) ret = x->to_list ();
    p.set_silent (old_silent);
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  for_t::publish (publish_t p, status_ev_t ev) const
  {
    tvars {
      ptr<expr_list_t> v;
      xpub_status_t status (XPUB_STATUS_OK);
      xpub_status_t res;
      size_t i;
      ptr<expr_t> row;
      ptr<expr_dict_t> locals;
      size_t stcksz;
    }

    v = eval_list (p);
    if (!v && !_empty) {
      
      strbuf b ("for: second argument is not an iterable vector");
      p.report_error (b, _location);
      
    } else if (!v || v->size () == 0) {
      
      if (_empty) {
	twait { _empty->publish (p, mkevent (status)); }
      }
      
    } else {
      
      assert (v);
      locals = expr_dict_t::alloc ();
      
      for (i = 0; i < v->size (); i++) {
	
	row = (*v)[i];
	if (!row) {
	  strbuf b ("for: row %zu is undefined!", i);
	  p.report_error (b, _location);
	} else if (!_body) {
	  // noop
	} else {
	  locals->replace (_iter, row);
	  stcksz = p.env ()->push_locals (locals);
	  twait { _body->publish (p, mkevent (res)); }
	  if (res.status != XPUB_STATUS_OK) {
	    status = res;
	  }
	  p.env ()->pop_to (stcksz);
	}
      }
    }
    ev->trigger (status);
  }

  //-----------------------------------------------------------------------

  bool
  if_t::might_block () const 
  {
    bool mb = false;
    if (!_might_block.is_set ()) {
      for (size_t i = 0;  !mb && _clauses && _clauses->size (); i++) {
	if ((*_clauses)[i]->might_block ()) {
	  mb = true;
	}
      }
      _might_block.set (mb);
    } else {
      mb = _might_block.value ();
    }
    return mb;
  }

  //-----------------------------------------------------------------------

  bool
  if_clause_t::might_block () const
  {
    return _body && _body->might_block ();
  }

  //-----------------------------------------------------------------------

  bool
  if_t::publish_nonblock (publish_t p) const
  {
    bool ret = false;

    // XXX - can do better than this.  This means we'll go into blocking
    // land if ANY clause of the if statement blocks.  It should be
    // that we'll go into blocking code only if the selected clause needs
    // to block.
    if (!might_block ()) {
      ptr<const zone_t> ne = find_clause (p);
      if (ne) {
	ret = ne->publish_nonblock (p);
      } else {
	ret = true;
      }
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  if_t::publish (publish_t p, status_ev_t ev) const
  {
    tvars {
      xpub_status_t status (XPUB_STATUS_OK);
      ptr<const zone_t> z;
    }
    if ((z = find_clause (p))) {
      twait { z->publish (p, mkevent (status)); }
    }
    ev->trigger (status);
  }

  //-----------------------------------------------------------------------

  bool
  if_clause_t::fits (publish_t p) const
  {
    return (!_expr || _expr->eval_as_bool (p));
  }

  //-----------------------------------------------------------------------

  ptr<const zone_t>
  if_t::find_clause (publish_t p) const
  {
    size_t len = _clauses ? _clauses->size () : size_t (0) ;
    ptr<const if_clause_t> c;
    ptr<const zone_t> ret;
    bool found = false;

    for (size_t i = 0; !found && i < len; i++) {
      if ((c = (*_clauses)[i]) && (c->fits (p))) {
	found = true;
	ret = c->body ();
      }
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  print_t::add (ptr<pub3::expr_list_t> l)
  {
    bool ret = true;
    if (!_args) {
      _args = l;
    } else {
      ret = false;
    }
    return ret;
  }
  
  //-----------------------------------------------------------------------
  
  bool
  print_t::publish_nonblock (publish_t p) const
  {
    str s;
    for (size_t i = 0; _args && i < _args->size (); i++) {
      ptr<const expr_t> x = (*_args)[i];
      if (x && (s = x->eval_as_str (p))) {
	p.output (s);
      }
    }
    return true;
  }
  
  //-----------------------------------------------------------------------

  bool
  decl_block_t::is_static () const
  {
    bool sttc = true;
    if (!_static.is_set ()) {
      for (size_t i = 0; sttc && _bindings && i < _bindings->size (); i++) {
	const binding_t &b = (*_bindings)[i];
	if (b.expr () && !b.expr ()->is_static ()) {
	  sttc = false;
	}
      }
      _static.set (sttc);
    }
    return _static.value ();
  }

  //-----------------------------------------------------------------------

  void
  decl_block_t::add (ptr<bindlist_t> l) 
  {
    _bindings = l;
    _tab = New refcounted<bindtab_t> ();
    for (size_t i = 0; l && i < l->size (); i++) {
      binding_t &b = (*l)[i];
      _tab->insert (b.name (), b.expr ());
    }
  }

  //-----------------------------------------------------------------------

  bool
  decl_block_t::publish_nonblock (publish_t p) const
  {
    if (is_static ()) {
      p.env ()->push_bindings (cow_bindtab_t::alloc (_tab), get_decl_type ());
    } else {
      ptr<bindtab_t> frame = New refcounted<bindtab_t> ();
      p.env ()->push_bindings (frame, get_decl_type ());

      // Construct the stack frame as we go! This way we can
      // use the previous binding in evaluating the next. This
      // is quite clever, check it.
      for (size_t i = 0; _bindings && i < _bindings->size (); i++) {
	const binding_t &b = (*_bindings)[i];
	ptr<expr_t> nv;
	ptr<const expr_t> cx;
	ptr<mref_t> r;
	if ((cx = b.expr ()) && (r = cx->eval_to_ref (p))) {
	  nv = r->get_value ();
	}
	frame->insert (b.name (), nv);
      }
    }
    return true;
  }

  //-----------------------------------------------------------------------

  bool
  expr_statement_t::publish_nonblock (publish_t p) const
  {
    // Evaluate for any side effects.
    if (_expr) { (void) _expr->eval_to_ref (p); }
    return true;
  }

  //-----------------------------------------------------------------------
  
};

