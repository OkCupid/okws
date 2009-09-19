#include "pub3parse.h"
#include "pub_parse.h"

namespace pub3 {

  // ===================================================== parser_t =======

  ptr<parser_t> g_current;

  //-----------------------------------------------------------------------

  ptr<parser_t> parser_t::current () { return g_current; }
  void parser_t::set_current (ptr<parser_t> p) { g_current = p; }
  parser_t::parser_t (str f) : _location (f, 1) {}
  lineno_t parser_t::location () const { return _location._lineno; }
  void parser_t::inc_lineno (lineno_t l) { _location._lineno += l; }
  const location_t &location () const { return _location; }

  // =============================================== json_parser_t ========

  json_parser_t::json_parser_t () : parser_t ("<json>") {}

  //-----------------------------------------------------------------------

  bool
  json_parser_t::set_expr_output (ptr<expr_t> e)
  {
    _out = x;
    return true;
  }

  //-----------------------------------------------------------------------
  
  ptr<expr_t>
  json_parser_t::parse (const str &in)
  {
    _out = NULL;
    yy_parse_json (in);
    yyparse ();
    flex_cleanup ();
    ptr<expr_t> ret = _out;
    _out = NULL;
    return ret;
  }
  
  // ================================================ pub_parser_t ======

  bool
  pub_parser_t::set_zone_output (ptr<zone_t> z)
  {
    _out = z;
    return true;
  }

  //---------------------------------------------------------------------
};

