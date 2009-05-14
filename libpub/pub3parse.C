#include "pub3parse.h"
#include "pub_parse.h"

static ptr<pub3::expr_t> g_json_out;

//-----------------------------------------------------------------------

void 
pub3::json_parser_t::set_output(ptr<pub3::expr_t> e) 
{
  g_json_out = e;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
pub3::json_parser_t::parse (const str &in)
{
  yy_parse_json (in);
  yyparse ();
  flex_cleanup ();

  return g_json_out;
}

//-----------------------------------------------------------------------
