// -*-c++-*-

#include "pub3ast.h"
#include "pub3parse.h"
#include "pub3out.h"

namespace pub3 {

  //====================================================== zone_t =========

  bool zone_t::handle_control (eval_t *p) const
  { return p->control ()->handle_zone (); }

  //================================================== ast_node_t =========

  static const location_t &location () 
  { return parser_t::current ()->location (); }

  //-----------------------------------------------------------------------

  static location_t location (lineno_t l) 
  { return parser_t::current ()->location (l); }

  //-----------------------------------------------------------------------

  tamed void 
  ast_node_t::publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      xpub_status_t status;
    }

    p->set_lineno (lineno ());

    if (!might_block ()) {
      status = v_publish_nonblock (p);
    } else {
      twait { v_publish (p, mkevent (status)); }
    }
    ev->trigger (status);
  }

  //-----------------------------------------------------------------------

  void
  ast_node_t::propogate_metadata (ptr<const metadata_t> md)
  {
    _location.set_filename (md->jailed_filename (), true);
  }

  //-----------------------------------------------------------------------

  status_t ast_node_t::publish_nonblock (eval_t *p) const
  { return v_publish_nonblock (p); }

  //-----------------------------------------------------------------------

  bool
  ast_node_t::might_block () const
  {
    if (!_might_block.is_set ()) {
      bool mb = might_block_uncached ();
      _might_block.set (mb);
    }
    return _might_block.value ();
  }

  //-----------------------------------------------------------------------

  void
  ast_node_t::v_publish (eval_t *p, status_ev_t ev, ptr<closure_t> dum) const
  { 
    return ev->trigger (v_publish_nonblock (p));
  }

  //============================================= zone_raw_t ==============

  zone_raw_t::zone_raw_t (location_t l, zstr z) : zone_t (l), _data (z) {}

  //-----------------------------------------------------------------------

  ptr<zone_raw_t> zone_raw_t::alloc (location_t l, zstr z) 
  { return New refcounted<zone_raw_t> (l, z); }

  //-----------------------------------------------------------------------

  void 
  zone_raw_t::v_dump (dumper_t *d) const
  {
    d->dump ("<binary data>", true);
  }

  //-----------------------------------------------------------------------

  bool zone_raw_t::might_block_uncached () const { return false; }

  //-----------------------------------------------------------------------

  status_t 
  zone_raw_t::v_publish_nonblock (eval_t *p) const
  {
    p->output (_data);
    return XPUB_STATUS_OK;
  }

  //-----------------------------------------------------------------------
 
  void
  zone_raw_t::v_publish (eval_t *p, status_ev_t ev, ptr<closure_t> d) const
  { ev->trigger (v_publish_nonblock (p)); }

  //============================================= zone_conainer_t =========

  void
  zone_container_t::propogate_metadata (ptr<const metadata_t> md)
  {
    zone_t::propogate_metadata (md);
    for (size_t i = 0; i < _children.size (); i++) {
      _children[i]->propogate_metadata (md);
    }
  }

  //============================================= zone_html_t =============
  
  bool
  zone_html_t::might_block_uncached () const
  {
    bool bl = false;
    for (size_t i = 0; !bl && i < _children.size (); i++) {
      if (_children[i]->might_block ()) {
	bl = true;
      }
    }
    return bl;
  }

  //-----------------------------------------------------------------------

  status_t
  zone_html_t::v_publish_nonblock (eval_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    for (size_t i = 0; handle_control (p) && i < _children.size (); i++) {
      status_t tmp = _children[i]->publish_nonblock (p);
      if (tmp.status != XPUB_STATUS_OK) {
	ret = tmp;
      }
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  zone_html_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      status_t ret (XPUB_STATUS_OK);
      status_t tmp;
      size_t i;
    }
    for (i = 0; handle_control (p) && i < _children.size (); i++) {
      twait { _children[i]->publish (p, mkevent (tmp)); }
      if (tmp.status != XPUB_STATUS_OK) {
	ret = tmp;
      }
    }
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  zone_html_t::zone_html_t (location_t l) : 
    zone_container_t (l) {}

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
  zone_html_t::add_boundary (str s)
  {
    bool on = true;
    size_t offset = 1;            // skip past '<'

    if (s[offset] == '/') {
      on = false;
      offset ++;                  // skip past '/'
    }

    str tag = s + offset;

    _children.push_back (zone_wss_boundary_t::alloc (on, tag));

    // can optimize this; no need to push_zone_text since we know
    // that the proceeding will not be text (since it's a wss boundary!)
    add (s);
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

  void 
  zone_html_t::v_dump (dumper_t *d) const
  {
    for (size_t i = 0; i < _children.size (); i++) {
      s_dump (d, "zone:", _children[i]);
    }
  }

  //============================================== zone_wss_boundary_t ====

  void
  zone_wss_boundary_t::v_dump (dumper_t *d) const
  {
    d->dump (strbuf ("zone_wss_boundary_t: %s", _on ? "on" : "off"));
  }

  //-----------------------------------------------------------------------

  ptr<zone_wss_boundary_t>
  zone_wss_boundary_t::alloc (bool on, str tag)
  {
    return New refcounted<zone_wss_boundary_t> (location (), on, tag);
  }

  //-----------------------------------------------------------------------

  status_t
  zone_wss_boundary_t::v_publish_nonblock (eval_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    if (!p->out ()->wss_boundary (_on, _tag)) {
      p->report_error ("Unbalanced </script>, </textarea> OR </pre>", 
		       _location);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  zone_wss_boundary_t::v_publish (eval_t *p, status_ev_t ev, 
				  ptr<closure_t> dummy) const
  {
    ev->trigger (v_publish_nonblock (p)); 
  }

  //============================================== zone_text_t ============

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

  status_t 
  zone_text_t::v_publish_nonblock (eval_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    cook_text ();
    p->output (_original, _wss);
    return ret;
  }

  //-----------------------------------------------------------------------

  void zone_text_t::v_publish (eval_t *p, status_ev_t ev, 
			       ptr<closure_t> dummy) const
  { ev->trigger (v_publish_nonblock (p)); }

  //-----------------------------------------------------------------------
  
  void zone_text_t::cook_text () const 
  { 
    if (!_original && _b.len ()) { _original = str (_b); }
    if (!_wss && _original) { _wss = html_wss (_original.to_str ()); }
  }

  //-----------------------------------------------------------------------

  void
  zone_text_t::v_dump (dumper_t *d) const
  {
    cook_text ();
    strbuf b;
    if (_original) {
      str tmp = _original;
      b << "orig: " << tmp;
      d->dump (b, true);
    }
    if (_wss) {
      str tmp = _wss;
      b << "wss: " << tmp;
      d->dump (b, true);
    }
  }

  //=======================================================================

  ptr<zone_inline_expr_t> zone_inline_expr_t::alloc (ptr<expr_t> x)
  { return New refcounted<zone_inline_expr_t> (location (), x); }

  //----------------------------------------------------------------------

  zone_inline_expr_t::zone_inline_expr_t (location_t l, ptr<expr_t> x)
    : zone_t (l), _expr (x) {}

  //----------------------------------------------------------------------

  bool
  zone_inline_expr_t::might_block_uncached () const
  {
    return _expr && _expr->might_block ();
  }

  //----------------------------------------------------------------------

  tamed void
  zone_inline_expr_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      status_t res (XPUB_STATUS_OK);
      str s;
      bool dowarn, old_loud;
    }

    dowarn = p->opts () & P_WARN_INLINE_NULL;
    if (dowarn) { old_loud = p->set_loud (true); }

    twait { _expr->pub_as_str (p, mkevent (s)); }
    if (s) { p->output (s); }
    else if (dowarn) { null_warn (p); }

    if (dowarn) { p->set_loud (old_loud); }

    ev->trigger (res);
  }

  //----------------------------------------------------------------------

  void
  zone_inline_expr_t::null_warn (eval_t *p) const
  {
    p->report_error ("Inline expression %{} evaluates to NULL", _location);
  }

  //----------------------------------------------------------------------

  status_t
  zone_inline_expr_t::v_publish_nonblock (eval_t *p) const
  {
    bool dowarn = false, old_loud = false;
    status_t res (XPUB_STATUS_OK);

    dowarn = p->opts () & P_WARN_INLINE_NULL;
    if (dowarn) { old_loud = p->set_loud (true); }

    str s = _expr->eval_as_str (p);
    if (s) { p->output (s); }
    else if (dowarn) { null_warn (p); }

    if (dowarn) { p->set_loud (old_loud); }
    return res;
  }

  //-----------------------------------------------------------------------

  void
  zone_inline_expr_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "expr:", _expr);
  }

  //-----------------------------------------------------------------------

  void
  zone_inline_expr_t::propogate_metadata (ptr<const metadata_t> md)
  {
    if (_expr) _expr->propogate_metadata (md);
  }

  //======================================================================

  ptr<zone_pub_t> zone_pub_t::alloc ()
  { return New refcounted<zone_pub_t> (location ()); }

  //----------------------------------------------------------------------

  void zone_pub_t::reserve () { _statements.push_back (NULL); }
  void zone_pub_t::add (ptr<statement_t> s) { _statements.push_back (s); }

  //----------------------------------------------------------------------

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

  bool
  zone_pub_t::might_block_uncached () const
  {
    bool bl = false;
    for (size_t i = 0; i < _statements.size () && !bl; i++) {
      if (_statements[i]->might_block ()) { bl = true; }
    }
    return bl;
  }

  //-----------------------------------------------------------------------

  status_t 
  zone_pub_t::v_publish_nonblock (eval_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    for (size_t i= 0; handle_control (p) && i < _statements.size (); i++) {
      status_t tmp = _statements[i]->publish_nonblock (p);
      if (tmp.status != XPUB_STATUS_OK) {
	ret = tmp;
      }
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  zone_pub_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      status_t ret (XPUB_STATUS_OK);
      status_t tmp;
      size_t i;
    }
    for (i = 0; handle_control (p) && i < _statements.size (); i++) {
      twait { _statements[i]->publish (p, mkevent (tmp)); }
      if (tmp.status != XPUB_STATUS_OK) {
	ret = tmp;
      }
    }
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  void
  zone_pub_t::v_dump (dumper_t *d) const
  {
    for (size_t i = 0; i < _statements.size (); i++) {
      s_dump (d, "statement:", _statements[i]);
    }
  }

  //-----------------------------------------------------------------------

  void
  zone_pub_t::propogate_metadata (ptr<const metadata_t> md)
  {
    zone_t::propogate_metadata (md);
    for (size_t i = 0; i < _statements.size (); i++) {
      _statements[i]->propogate_metadata (md);
    }
  }

  //======================================================================

  ptr<statement_zone_t> 
  statement_zone_t::alloc (ptr<zone_t> z)
  {
    return New refcounted<statement_zone_t> (location (), z);
  }

  //---------------------------------------------------------------------

  statement_zone_t::statement_zone_t (location_t l, ptr<zone_t> z)
    : statement_t (l), _zone (z) {}

  //---------------------------------------------------------------------
  
  bool statement_zone_t::might_block_uncached () const
  { return _zone && _zone->might_block (); }

  //---------------------------------------------------------------------

  status_t statement_zone_t::v_publish_nonblock (eval_t *p) const
  { return _zone->publish_nonblock (p); }

  //---------------------------------------------------------------------

  tamed void
  statement_zone_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      status_t status;
    }
    twait { _zone->publish (p, mkevent (status)); }
    return ev->trigger (status);
  }
  
  //---------------------------------------------------------------------

  void
  statement_zone_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "zone:", _zone);
  }

  //-----------------------------------------------------------------------

  void
  statement_zone_t::propogate_metadata (ptr<const metadata_t> md)
  {
    ast_node_t::propogate_metadata (md);
    if (_zone) { _zone->propogate_metadata (md); }
  }

  //======================================= expr_statement_t =============

  status_t
  expr_statement_t::v_publish_nonblock (eval_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    // Evaluate for any side effects.
    if (_expr) { (void) _expr->eval_to_ref (p); }
    return ret;
  }

  //----------------------------------------------------------------------

  bool expr_statement_t::might_block_uncached () const
  { return _expr->might_block (); }

  //----------------------------------------------------------------------

  tamed void
  expr_statement_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      status_t ret (XPUB_STATUS_OK);
      ptr<mref_t> dummy;
    }
    if (_expr) {
      twait { _expr->pub_to_ref (p, mkevent (dummy)); }
    }
    ev->trigger (ret);
  }
  
  //----------------------------------------------------------------------
  
  expr_statement_t::expr_statement_t (ptr<expr_t> x, location_t l)
    : statement_t (l), _expr (x) {}

  //----------------------------------------------------------------------

  ptr<expr_statement_t> expr_statement_t::alloc (ptr<expr_t> x) 
  { return New refcounted<expr_statement_t> (x, location ()); }

  //----------------------------------------------------------------------

  void
  expr_statement_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "expr:", _expr);
  }

  //-----------------------------------------------------------------------

  void
  expr_statement_t::propogate_metadata (ptr<const metadata_t> md)
  {
    ast_node_t::propogate_metadata (md);
    if (_expr) { _expr->propogate_metadata (md); }
  }

  //============================================== while_t ================

  ptr<while_t> while_t::alloc (lineno_t l) 
  { 
    return New refcounted<while_t> (location (l));
  }

  //-----------------------------------------------------------------------

  bool while_t::add_cond (ptr<expr_t> x) { _cond = x; return true; }
  bool while_t::add_body (ptr<zone_t> z) { _body = z; return true; }
  
  //-----------------------------------------------------------------------

  bool
  while_t::handle_control (eval_t *p) const
  {
    return p->control ()->handle_forloop ();
  }

  //-----------------------------------------------------------------------

  void while_t::reset_control (eval_t *p) const
  { p->control ()->reset_forloop ();  }

  //-----------------------------------------------------------------------

  bool
  while_t::might_block_uncached () const
  {
    bool b1 = _cond && _cond->might_block ();
    bool b2 = _body && _body->might_block ();
    return b1 || b2;
  }

  //-----------------------------------------------------------------------

  status_t 
  while_t::v_publish_nonblock (eval_t *p) const
  {
    status_t status (XPUB_STATUS_OK);

    reset_control (p);

    while (handle_control (p) && _cond && _cond->eval_as_bool (p)) {
      if (_body) {
	size_t stcksz = p->env ()->stack_size ();
	status_t tmp = _body->publish_nonblock (p);
	p->env ()->pop_to (stcksz);
      }
    }
    return status;
  }

  //-----------------------------------------------------------------------

  tamed void
  while_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      xpub_status_t status (XPUB_STATUS_OK);
      xpub_status_t tmp;
      size_t stcksz;
      bool go (true);
    }

    reset_control (p);

    while (go && handle_control (p)) {

      if (!_cond) { go = false; }
      else { twait { _cond->pub_as_bool (p, mkevent (go)); } }

      if (go && _body) {
	stcksz = p->env ()->stack_size ();
	twait { _body->publish (p, mkevent (tmp)); }
	p->env ()->pop_to (stcksz);
      }
    }
    ev->trigger (status);
  }

  //-----------------------------------------------------------------------

  void
  while_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "cond:", _cond);
    s_dump (d, "body:", _body);
  }

  //-----------------------------------------------------------------------

  void
  while_t::propogate_metadata (ptr<const metadata_t> md)
  {
    ast_node_t::propogate_metadata (md);
    if (_cond) { _cond->propogate_metadata (md); }
    if (_body) { _body->propogate_metadata (md); }
  }

  //================================================ for_t ================

  bool
  for_t::add_params (ptr<expr_list_t> l)
  {
    bool ret = true;
    if (!l || l->size () != 2) {
      parse_error ("for: takes 2 arguments (simple identifier and array)\n");
      ret = false;
    } else if (!(_iter = (*l)[0]->to_identifier ()) || !_iter.len ()) {
      parse_error ("for: argument 1 must be an identifier\n");
      ret = false;
    } else {
      _arr = (*l)[1];
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<for_t> for_t::alloc (lineno_t l) 
  { 
    return New refcounted<for_t> (location (l));
  }

  //-----------------------------------------------------------------------

  bool for_t::add_body (ptr<zone_t> z) { _body = z; return true; }
  bool for_t::add_empty (ptr<zone_t> z) { _empty = z; return true; }
  
  //-----------------------------------------------------------------------

  ptr<expr_list_t>
  for_t::eval_list (eval_t *p) const
  {
    bool old_silent = p->set_silent (true);
    ptr<mref_t> m = _arr->eval_to_ref (p);
    ptr<expr_t> x;
    if (m) x = m->get_value ();
    ptr<expr_list_t> ret;
    if (x) ret = x->to_list ();
    p->set_silent (old_silent);
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  for_t::pub_list (eval_t *p, xlev_t ev) const
  {
    tvars {
      bool old_silent (p->set_silent (true));
      ptr<mref_t> m;
      ptr<expr_t> x;
      ptr<expr_list_t> ret;
    }
    twait { _arr->pub_to_ref (p, mkevent (m)); }
    if (m) x = m->get_value ();
    if (x) ret = x->to_list ();
    p->set_silent (old_silent);
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  void
  for_t::err_empty (eval_t *p) const
  {
      strbuf b ("for: second argument is not an iterable vector");
      p->report_error (b, _location);
  }

  //-----------------------------------------------------------------------

  void
  for_t::err_badrow (eval_t *p, size_t i) const
  {
    strbuf b ("for: row %zu is undefined!", i);
    p->report_error (b, _location);
  }

  //-----------------------------------------------------------------------

  bool
  for_t::handle_control (eval_t *p) const
  {
    return p->control ()->handle_forloop ();
  }

  //-----------------------------------------------------------------------

  void for_t::reset_control (eval_t *p) const
  { p->control ()->reset_forloop ();  }

  //-----------------------------------------------------------------------

  bool
  for_t::might_block_uncached () const
  {
    bool b1 = _arr && _arr->might_block ();
    bool b2 = _empty && _empty->might_block ();
    bool b3 = _body && _body->might_block ();
    return b1 || b2 || b3;
  }

  //-----------------------------------------------------------------------

  status_t 
  for_t::v_publish_nonblock (eval_t *p) const
  {
    status_t status (XPUB_STATUS_OK);
    ptr<expr_list_t> v = eval_list (p);
    if (!v && !_empty) {
      err_empty (p);
    } else if (!v || v->size () == 0) {
      if (_empty) { status = _empty->publish_nonblock (p); }
    } else {
      
      assert (v);
      ptr<expr_dict_t> locals = expr_dict_t::alloc ();
      reset_control (p);
      
      for (size_t i = 0; handle_control (p) && i < v->size (); i++) {
	
	ptr<expr_t> row = (*v)[i];
	ptr<mref_t> rr;
	ptr<expr_t> rev;
	if (!row || !(rr = row->eval_to_ref (p)) || !(rev = rr->get_value ())) {
	  err_badrow (p, i);
	} else if (!_body) {
	  // noop
	} else {
	  locals->replace (_iter, rev);
	  size_t stcksz = p->env ()->push_locals (locals);
	  status_t tmp = _body->publish_nonblock (p);
	  if (tmp.status != XPUB_STATUS_OK) { status = tmp; }
	  p->env ()->pop_to (stcksz);
	  // In case the row was actually modified, we should reassign it.
	  (*v)[i] = rev;
	}
      }
    }
    return status;
  }

  //-----------------------------------------------------------------------

  tamed void
  for_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      ptr<expr_list_t> v;
      xpub_status_t status (XPUB_STATUS_OK);
      xpub_status_t res;
      size_t i;
      ptr<expr_t> row, rev;
      ptr<mref_t> rr;
      ptr<expr_dict_t> locals;
      size_t stcksz;
    }

    twait { pub_list (p, mkevent (v)); }

    if (!v && !_empty) {
      err_empty (p);
      
    } else if (!v || v->size () == 0) {
      
      if (_empty) {
	twait { _empty->publish (p, mkevent (status)); }
      }
      
    } else {
      
      assert (v);
      locals = expr_dict_t::alloc ();

      reset_control (p);

      for (i = 0; handle_control (p) && i < v->size (); i++) {
	
	row = (*v)[i];
	if (!row) {
	  err_badrow (p, i);
	} else { 
	  twait { row->pub_to_ref (p, mkevent (rr)); }
	  if (!rr || !(rev = rr->get_value ())) {
	    err_badrow (p, i);
	  } else if (!_body) {
	    // noop
	  } else {
	    locals->replace (_iter, rev);
	    stcksz = p->env ()->push_locals (locals);
	    twait { _body->publish (p, mkevent (res)); }
	    if (res.status != XPUB_STATUS_OK) { status = res; }
	    p->env ()->pop_to (stcksz);
	    // In case the row was actually modified, we should reassign it.
	    (*v)[i] = rev;
	  }
	}
      }
    }
    ev->trigger (status);
  }

  //-----------------------------------------------------------------------

  void
  for_t::v_dump (dumper_t *d) const
  {
    strbuf b;
    b << "iter: " << _iter;
    d->dump (b, true);
    s_dump (d, "arr:", _arr);
    s_dump (d, "body:", _body);
    s_dump (d, "empty:", _empty);
  }

  //-----------------------------------------------------------------------

  void
  for_t::propogate_metadata (ptr<const metadata_t> md)
  {
    ast_node_t::propogate_metadata (md);
    if (_arr) { _arr->propogate_metadata (md); }
    if (_body) { _body->propogate_metadata (md); }
    if (_empty) { _empty->propogate_metadata (md); }
  }

  //==================================================== if_t ==============

  bool
  if_t::might_block_uncached () const 
  {
    bool mb = false;
    for (size_t i = 0;  !mb && _clauses && i < _clauses->size (); i++) {
      if ((*_clauses)[i]->might_block ()) { mb = true; }
    }
    return mb;
  }

  //-----------------------------------------------------------------------

  void
  if_clause_t::propogate_metadata (ptr<const metadata_t> md)
  {
    if (_expr) { _expr->propogate_metadata (md); }
    if (_body) { _body->propogate_metadata (md); }
  }

  //-----------------------------------------------------------------------

  void
  if_t::propogate_metadata (ptr<const metadata_t> md)
  {
    ast_node_t::propogate_metadata (md);
    for (size_t i = 0; _clauses && i < _clauses->size (); i++) {
      (*_clauses)[i]->propogate_metadata (md);
    }
  }

  //-----------------------------------------------------------------------

  bool
  if_clause_t::might_block () const
  {
    if (!_might_block.is_set ()) {
      bool b1 = _expr && _expr->might_block ();
      bool b2 = _body && _body->might_block ();
      _might_block.set (b1 || b2);
    }
    return _might_block.value ();
  }

  //-----------------------------------------------------------------------

  void
  if_clause_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "expr:", _expr);
    s_dump (d, "body:", _body);
  }

  //-----------------------------------------------------------------------

  status_t 
  if_t::v_publish_nonblock (eval_t *p) const
  {
    ptr<const zone_t> ne = find_clause (p);
    status_t s (XPUB_STATUS_OK);
    if (ne) { s = ne->publish_nonblock (p); }
    return s;
  }

  //-----------------------------------------------------------------------

  tamed void
  if_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      xpub_status_t status (XPUB_STATUS_OK);
      ptr<const zone_t> z;
    }
    twait { find_clause (p, mkevent (z)); }
    if (z) {
      twait { z->publish (p, mkevent (status)); }
    }
    ev->trigger (status);
  }

  //-----------------------------------------------------------------------

  bool
  if_clause_t::fits (eval_t *p) const
  {
    return (!_expr || _expr->eval_as_bool (p));
  }

  //-----------------------------------------------------------------------

  tamed void
  if_clause_t::fits (eval_t *p, evb_t ev) const
  {
    tvars {
      bool ret (true);
    }
    if (_expr) {
      twait { _expr->pub_as_bool (p, mkevent (ret)); }
    } 
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------
  
  tamed void
  if_t::find_clause (eval_t *p, czev_t ev) const
  {
    tvars {
      size_t len;
      ptr<const if_clause_t> c;
      ptr<const zone_t> ret;
      bool found (false);
      size_t i;
    }

    len = _clauses ? _clauses->size () : size_t (0) ;

    for (i = 0; !found && i < len; i++) {
      if ((c = (*_clauses)[i])) {
	twait { c->fits (p, mkevent (found)); }
	if (found) {
	  ret = c->body ();
	}
      }
    }
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  ptr<const zone_t>
  if_t::find_clause (eval_t *p) const
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

  ptr<if_clause_t> if_clause_t::alloc () 
  { return New refcounted<if_clause_t> (plineno ()); }

  //-----------------------------------------------------------------------

  ptr<if_t> if_t::alloc (lineno_t l) 
  { return New refcounted<if_t> (location (l)); }

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

  void
  if_t::v_dump (dumper_t *d) const
  {
    for (size_t i = 0; i < _clauses->size (); i++) {
      s_dump (d, "clause:", (*_clauses)[i]);
    }
  }
  
  //========================================== decl_block_t =============

  bool
  decl_block_t::might_block_uncached () const
  {
    return _tab && _tab->might_block ();
  }

  //--------------------------------------------------------------------

  void
  decl_block_t::propogate_metadata (ptr<const metadata_t> md)
  {
    if (_tab) _tab->propogate_metadata (md);
  }


  //--------------------------------------------------------------------

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
    _tab = expr_dict_t::alloc();
    for (size_t i = 0; l && i < l->size (); i++) {
      binding_t &b = (*l)[i];
      _tab->insert (b.name (), b.expr ());
    }
  }

  //-----------------------------------------------------------------------

  tamed void
  decl_block_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      status_t ret (XPUB_STATUS_OK);
      size_t i;
      ptr<expr_t> nv;
      ptr<const expr_t> cx;
      ptr<bindtab_t> frame;
      str nm;
      binding_t b;
      env_t::layer_type_t lt (_self->get_decl_type ());
      bool locals (lt == env_t::LAYER_LOCALS);
    }

    if (is_static () && locals) {

      p->env ()->push_locals (cow_bindtab_t::alloc (_tab));
    } else {
      frame = p->push_bindings (lt);

      // Construct the stack frame as we go! This way we can
      // use the previous binding in evaluating the next. This
      // is quite clever, check it.
      for (i = 0; _bindings && i < _bindings->size (); i++) {
	nv = NULL;

        b = (*_bindings)[i];
	nm = b.name ();

	if ((cx = b.expr ())) {
	  twait { cx->pub_to_mval (p, mkevent (nv)); }
	}
	if (cx || locals) {
	  if (!nv) nv = expr_null_t::alloc ();
	  frame->insert (nm, nv);
	}
      }
      p->env ()->push_references (_bindings, lt);
    }
    ev->trigger (ret);
  }


  //-----------------------------------------------------------------------

  status_t
  decl_block_t::v_publish_nonblock (eval_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    env_t::layer_type_t lt = get_decl_type ();
    bool locals (lt == env_t::LAYER_LOCALS);

    if (is_static () && locals) {
        
      p->env ()->push_locals (cow_bindtab_t::alloc (_tab));
    } else {
      ptr<bindtab_t> frame = p->push_bindings (lt);

      // Construct the stack frame as we go! This way we can
      // use the previous binding in evaluating the next. This
      // is quite clever, check it.
      for (size_t i = 0; _bindings && i < _bindings->size (); i++) {
	const binding_t &b = (*_bindings)[i];
	ptr<expr_t> nv;
	ptr<const expr_t> cx;

	if ((cx = b.expr ())) { nv = cx->eval_to_mval (p); }
	if (cx || locals) {
	  if (!nv) nv = expr_null_t::alloc ();
	  frame->insert (b.name (), nv);
	}
      }

      // universals are both set at the base level, and pushed
      // on as a layer of empty references.
      p->env ()->push_references (_bindings, lt);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  decl_block_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "bindings", _tab);
  }

  //============================== locals / universals ====================

  ptr<locals_t> locals_t::alloc (lineno_t l) 
  { return New refcounted<locals_t> (location (l)); }

  //-----------------------------------------------------------------------

  ptr<universals_t> universals_t::alloc (lineno_t l) 
  { return New refcounted<universals_t> (location (l)); }

  //-----------------------------------------------------------------------

  ptr<globals_t> globals_t::alloc (lineno_t l)
  { return New refcounted<globals_t> (location (l)); }

  //============================================= switch_t ================

  void case_list_t::add_case (ptr<case_t> c) { if (c) { push_back (c); } }
  void case_t::add_key (ptr<expr_t> x) { _key = x; }
  void case_t::add_zone (ptr<zone_t> z) { _zone = z; }
  bool case_t::might_block () const { return _zone && _zone->might_block (); }

  //-----------------------------------------------------------------------

  void
  switch_t::propogate_metadata (ptr<const metadata_t> md)
  {
    ast_node_t::propogate_metadata (md);
    if (_key) _key->propogate_metadata (md);
    if (_cases) _cases->propogate_metadata (md);
  }

  //-----------------------------------------------------------------------

  void
  case_list_t::propogate_metadata (ptr<const metadata_t> md)
  {
    for (size_t i = 0; i < size (); i++) {
      (*this)[i]->propogate_metadata (md);
    }
  }

  //-----------------------------------------------------------------------

  void
  case_t::propogate_metadata (ptr<const metadata_t> md)
  {
    if (_key) _key->propogate_metadata (md);
    if (_zone) _zone->propogate_metadata (md);
  }

  //-----------------------------------------------------------------------

  void
  case_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "key", _key);
    s_dump (d, "zone", _zone);
  }

  //-----------------------------------------------------------------------


  void switch_t::add_key (ptr<expr_t> x) { _key = x; }

  //-----------------------------------------------------------------------

  bool 
  switch_t::add_cases (ptr<case_list_t> l) 
  { 
    _cases = l; 
    return populate_cases ();
  }

  //-----------------------------------------------------------------------
  
  bool
  switch_t::populate_cases ()
  {
    _map.clear ();
    size_t n_defaults = 0;
    for (size_t i = 0; _cases && i < _cases->size (); i++) {
      ptr<case_t> c = (*_cases)[i];
      assert (c);
      ptr<const expr_t> k = c->key ();
      str ks;
      if (!k) {
	n_defaults ++;
	_default = c;
      } else if (k->is_null () || !(ks = k->to_switch_str ())) {
	_null = c;
      } else {
	_map.insert (ks, c);
      }
    }
    return (n_defaults <= 1);
  }
  
  //-----------------------------------------------------------------------
  
  ptr<const zone_t>
  switch_t::find_case (eval_t *pub) const
  {
    ptr<const zone_t> ret;
    str k;
    if (_key) {
      k = _key->eval_as_str (pub);
      if (k) {
	const ptr<case_t> *c = _map[k];
	if (c && *c) { ret = (*c)->zone (); }
      } else if (_null) {
	ret = _null->zone ();
      }
    }
    
    if (!ret && _default) { ret = _default->zone (); }
    
    if (!ret) {
      if (!k) k = "(null)";
      strbuf err ("no case found for key ='%s'", k.cstr ());
      pub->output_err (err, P_ERR_WARNING);
    }
    return ret;
  }

  //-----------------------------------------------------------------------
  
  bool
  switch_t::might_block_uncached () const
  {
    bool mb = false;
    for (size_t i = 0; !mb && _cases && i < _cases->size (); i++) {
      if ((*_cases)[i]->might_block ()) { mb = true; }
    }
    return mb;
  }

  //-----------------------------------------------------------------------

  status_t 
  switch_t::v_publish_nonblock (eval_t *pub) const
  {
    status_t ret (XPUB_STATUS_OK);
    ptr<const zone_t> zone = find_case (pub);
    if (zone) { zone->publish_nonblock (pub); }
    return ret;
  }
  
  //-----------------------------------------------------------------------

  tamed void 
  switch_t::v_publish (eval_t *pub, status_ev_t ev) const
  {
    tvars {
      status_t res (XPUB_STATUS_OK);
      ptr<const zone_t> zone;
    }
    if ((zone = find_case (pub))) {
      twait { zone->publish (pub, mkevent (res)); }
    }
    ev->trigger (res);
  }

  //--------------------------------------------------------------------

  ptr<switch_t> switch_t::alloc () 
  { return New refcounted<switch_t> (location ()); }

  //--------------------------------------------------------------------

  ptr<case_t> case_t::alloc () { return New refcounted<case_t> (plineno ()); }
  ptr<case_list_t> case_list_t::alloc () 
  { return New refcounted<case_list_t> (); }

  //-----------------------------------------------------------------------

  void
  switch_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "key:", _key);
    for (size_t i = 0; i < _cases->size (); i++) {
      s_dump (d, "case:", (*_cases)[i]);
    }
  }

  //============================================== include_t ==============

  ptr<include_t> include_t::alloc () 
  { return New refcounted<include_t> (location ()); }
  ptr<load_t> load_t::alloc () { return New refcounted<load_t> (location ()); }

  //-----------------------------------------------------------------------

  void
  include_t::propogate_metadata (ptr<const metadata_t> md)
  {
    if (_file) _file->propogate_metadata (md);
    if (_dict) _dict->propogate_metadata (md);
  }

  //-----------------------------------------------------------------------

  status_t
  include_t::v_publish_nonblock (eval_t *p) const
  {
      str fn;

      if (_file) {
          fn = _file->eval_as_str(p);
      } else
          fn = "unknown";

    p->report_error (str("include skipped in nonblock context: ") << fn, 
                     _location);
    return status_t ();
  }

  //-----------------------------------------------------------------------

  tamed void
  include_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      status_t rs (XPUB_STATUS_OK);
      str fn;
      bool mz;
      ptr<control_t> ctrl;
      ptr<bind_interface_t> bi;
    }

    if (_file) { twait { _file->pub_as_str (p, mkevent (fn)); } }
    if (_dict) { twait { _dict->pub_to_bindtab (p, mkevent (bi)); } }
    
    if (!fn || fn.len () == 0) {
      str e = "include: cannot evaluate filename";
      p->report_error (e, _location);
    } else if (_dict && !bi) {
      str e = "include: cannot evalute parameters";
      p->report_error (e, _location);
    } else {
      fn = trunc_after_null_byte (fn);
      mz = p->push_muzzle (muzzle_output ());
      ctrl = p->push_control ();
      twait { p->publish (fn, _location, bi, mkevent (rs)); }
      p->restore_control (ctrl);
      p->pop_muzzle (mz);
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
      if (l->size () == 2) _dict = (*l)[1];
    } else {
      str f = fnname ();
      err = strbuf ("%s takes 1 or 2 arguments; a filename and an optional "
		    "binding list", f.cstr ());
      if (errp) *errp = err;
      ret = false;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  include_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "file:", _file);
    s_dump (d, "dict:", _dict);
  }

  //============================================== print_t ================

  ptr<print_t> print_t::alloc (lineno_t l) 
  { return New refcounted<print_t> (l); }
  
  //-----------------------------------------------------------------------

  void
  print_t::propogate_metadata (ptr<const metadata_t> md)
  {
    ast_node_t::propogate_metadata (md);
    if (_args) _args->propogate_metadata (md);
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
  
  status_t 
  print_t::v_publish_nonblock (eval_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    str s;
    for (size_t i = 0; _args && i < _args->size (); i++) {
      ptr<const expr_t> x = (*_args)[i];
      if (x && (s = x->eval_as_str (p))) {
	p->output (s);
      }
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool print_t::might_block_uncached () const 
  { return _args && _args->might_block (); }

  //-----------------------------------------------------------------------

  tamed void
  print_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      status_t ret (XPUB_STATUS_OK);
      str s;
      size_t i;
      ptr<const expr_t> x;
    }

    for (i = 0; _args && i < _args->size (); i++) {
      x = (*_args)[i];
      twait { x->pub_as_str (p, mkevent (s)); }
      if (s) { p->output (s); }
    }
    ev->trigger (ret);
  }

  //----------------------------------------------------------------------

  void
  print_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "args", _args);
  }

  //=============================================== return_t =============

  ptr<return_t> return_t::alloc (ptr<expr_t> x)
  { return New refcounted<return_t> (plineno (), x); }

  //----------------------------------------------------------------------

  bool return_t::might_block_uncached () const
  { return _val && _val->might_block (); }
  
  //----------------------------------------------------------------------

  void
  return_t::propogate_metadata (ptr<const metadata_t> md)
  {
    ast_node_t::propogate_metadata (md);
    if (_val) _val->propogate_metadata (md);
  }

  //----------------------------------------------------------------------

  status_t
  return_t::v_publish_nonblock (eval_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    ptr<const expr_t> x;
    if (_val) { x = _val->eval_to_val (p); }
    if (!x) { x = expr_null_t::alloc (); }
    p->control ()->set_rtrn (x);
    return ret;
  }
  
  //----------------------------------------------------------------------

  tamed void
  return_t::v_publish (eval_t *p, status_ev_t ev) const
  {
    tvars {
      status_t ret (XPUB_STATUS_OK);
      ptr<const expr_t> x;
    }
    if (_val) { twait { _val->pub_to_val (p, mkevent (x)); } }
    if (!x) { x = expr_null_t::alloc (); }

    p->control ()->set_rtrn (x);

    ev->trigger (ret);
  }

  //----------------------------------------------------------------------

  void
  return_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "val:", _val);
  }

  //=============================================== break_t ==============

  ptr<break_t> break_t::alloc () 
  { return New refcounted<break_t> (plineno ()); }
  
  //----------------------------------------------------------------------
  
  status_t
  break_t::v_publish_nonblock (eval_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    p->control ()->set_break (true);
    return ret;
  }

  //----------------------------------------------------------------------

  void break_t::v_publish (eval_t *p, status_ev_t ev, ptr<closure_t> d) const
  { ev->trigger (v_publish_nonblock (p)); }

  //======================================== exit_t ==================

  ptr<exit_t> exit_t::alloc ()
  { return New refcounted<exit_t> (plineno ()); }

  //----------------------------------------------------------------------
  
  status_t
  exit_t::v_publish_nonblock (eval_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    p->control ()->set_exit (true);
    return ret;
  }

  //----------------------------------------------------------------------

  void exit_t::v_publish (eval_t *p, status_ev_t ev, ptr<closure_t> d) const
  { ev->trigger (v_publish_nonblock (p)); }

  //======================================== continue_t ==================

  ptr<continue_t> continue_t::alloc ()
  { return New refcounted<continue_t> (plineno ()); }

  //----------------------------------------------------------------------
  
  status_t
  continue_t::v_publish_nonblock (eval_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    p->control ()->set_continue (true);
    return ret;
  }

  //----------------------------------------------------------------------

  void continue_t::v_publish (eval_t *p, status_ev_t ev, ptr<closure_t> d) 
    const
  { ev->trigger (v_publish_nonblock (p)); }

  //======================================================================
};

