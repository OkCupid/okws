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
  g_json_out = NULL;
  yy_parse_json (in);
  yyparse ();
  flex_cleanup ();
  ptr<pub3::expr_t> ret = g_json_out;
  g_json_out = NULL;

  return ret;
}

//-----------------------------------------------------------------------
