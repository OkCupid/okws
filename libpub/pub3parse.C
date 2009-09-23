#include "pub3parse.h"
#include "pub_parse.h"

//=======================================================================

// dealloc space after each run
#ifdef HAVE_RECENT_FLEX
extern int yylex_destroy(void);
void flex_cleanup() { yylex_destroy (); }
#else
void flex_cleanup()
{
  warn << "XXX Flex cleanup can't run! Leaking memory!! XXX\n";
}
#endif

//=======================================================================

namespace pub3 {

  // =================================================== location_t =======

  str
  location_t::to_str () const
  {
    strbuf b (_filename);
    if (_lineno) {
      b << ":" << _lineno;
    }
    return b;
  }

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

  FILE *
  pub_parser_t:open_file (const str &f)
  {
    FILE *ret = NULL;
    struct stat sb;
    if (stat (rfn.cstr (), &sb) != 0) {
      error ("no such file exists");
    } else if (!S_ISREG (sb.st_mode)) {
      error ("file exists but is not a regular file");
    } else if (!(ret = fopen (f.cstr (), "r"))) {
      error (strbuf ("open failed: %m\n"));
    }
    return ret;
  }

  //---------------------------------------------------------------------

  void
  pub_parser_t::error (str m)
  {
    strbuf b;
    s = _location.to_str ();
    b << s << ": " << m;
    _errors.push_back (b);
  }

  //---------------------------------------------------------------------
  
  ptr<file_t>
  pub_parser_t::parse (ptr<metadata_t> d)
  {
    ptr<file_t> ret;

    _errors.clear ();

    // Figure out the relative pathname, listed in the content file
    str jfn = d->jailed_filename ();

    // Figure out the real filename of this file, perhaps by resolving
    // the jail (in the case of simulated jail mode...)
    str rfn = d->real_filename ();

    // must do this before trying to open the file (or anything else
    // for that matter).
    _location.._filename = rfn;

    // Sanity check and call fopen()
    FILE *fp = open_file (rfn);
    if (fp) {
      yy_buffer_state *yb = yy_new_buffer (fp, ok_pub3_yy_buffer_size);
      yy_switch_to_buffer (yb);
      yyparse ();
      flex_cleanup ();
      yy_delete_buffer (yb);
      fclose (fp);
      ret = file_t::alloc (d, _out);
      _out = NULL;
    }


    return ret;
  }

  //---------------------------------------------------------------------

};

