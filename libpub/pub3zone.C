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

};

