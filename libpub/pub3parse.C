#include "pub3parse.h"
#include "pub_parse.h"

//=======================================================================

// dealloc space after each run
extern int yylex_destroy(void);
void flex_cleanup() { yylex_destroy (); }

//=======================================================================

namespace pub3 {

  // ===================================================== parser_t =======

  ptr<parser_t> g_current;

  //-----------------------------------------------------------------------

  ptr<parser_t> parser_t::current () { return g_current; }
  void parser_t::set_current (ptr<parser_t> p) { g_current = p; }
  parser_t::parser_t (str f) : _location (f, 1), _error (false) {}
  parser_t::parser_t () : _location (), _error (false) {}
  lineno_t parser_t::lineno () const { return _location._lineno; }
  void parser_t::inc_lineno (lineno_t l) { _location._lineno += l; }
  const location_t &parser_t::location () const { return _location; }

  //---------------------------------------------------------------------
  
  location_t
  parser_t::location (lineno_t l) const
  {
    location_t ret;
    ret._filename = _location._filename;
    ret._lineno = l;
    return ret;
  }

  //---------------------------------------------------------------------
  
  void
  parser_t::error (str m)
  {
    strbuf b;
    str s = _location.to_str ();
    b << s << ": " << m;
    get_errors ().push_back (b);
  }

  // =============================================== json_parser_t ========

  json_parser_t::json_parser_t () : parser_t ("<json>") {}

  //-----------------------------------------------------------------------

  bool
  json_parser_t::set_expr_output (ptr<expr_t> e)
  {
    _out = e;
    return true;
  }

  //-----------------------------------------------------------------------

  ptr<expr_t>
  json_parser_t::parse (const str &in)
  {
    ptr<json_parser_t> p = New refcounted<json_parser_t> ();
    ptr<expr_t> ret = p->mparse (in);
    return ret;
  }

  //-----------------------------------------------------------------------
  
  ptr<expr_t>
  json_parser_t::mparse (const str &in)
  {
    ptr<parser_t> old = current ();
    set_current (mkref (this));
    _location.set_filename ("<json>");
    
    _out = NULL;
    yy_parse_json (in);
    yyparse ();
    yy_pop_all ();
    flex_cleanup ();
    ptr<expr_t> ret = _out;
    if (error_condition ()) {
      ret = NULL;
    }
    _out = NULL;

    set_current (old);
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
  pub_parser_t::open_file (const str &f)
  {
    FILE *fp = NULL;
    struct stat sb;
    if (stat (f.cstr (), &sb) != 0) {
      error ("no such file exists", PARSE_ENOENT);
    } else if (!S_ISREG (sb.st_mode)) {
      error ("file exists but is not a regular file", PARSE_EIO);
    } else if (!(fp = fopen (f.cstr (), "r"))) {
      error (strbuf ("open failed: %m\n"), PARSE_EIO);
    }
    return fp;
  }

  //---------------------------------------------------------------------

  bool
  pub_parser_t::file2zstr (FILE *fp, zstr *out)
  {
    enum { BUFSZ = 8192 };
    char buf [BUFSZ];
    strbuf b;
    bool ret = false;

    while (!feof (fp) && !ferror (fp)) {
      size_t sz = fread (buf, 1, BUFSZ, fp);
      b.tosuio ()->copy (buf, sz);
    }
    
    if (ferror (fp)) {
      error (strbuf ("read failure in file: %m\n"), PARSE_EIO);
    } else {
      *out = zstr (str (b));
      ret = true;
    }
    return ret;
  }

  //---------------------------------------------------------------------

  const vec<str> &pub_parser_t::get_errors () const 
  { return _ret->get_errors (); }
  vec<str> &pub_parser_t::get_errors () { return _ret->get_errors (); }

  //---------------------------------------------------------------------
  
  void pub_parser_t::error (str d) { error (d, PARSE_EPARSE); }
  void pub_parser_t::error () { _ret->set_status (PARSE_EPARSE); }
  
  //---------------------------------------------------------------------

  void 
  pub_parser_t::error (str d, parse_status_t st)
  {
    _ret->set_status (st);
    parser_t::error (d);
  }

  //---------------------------------------------------------------------
  
  bool
  pub_parser_t::parse (ptr<metadata_t> d, parse_ret_t *r, opts_t opts)
  {
    ptr<parser_t> old = current ();
    set_current (mkref (this));

    // Figure out the relative pathname, listed in the content file
    str jfn = d->jailed_filename ();

    // Figure out the real filename of this file, perhaps by resolving
    // the jail (in the case of simulated jail mode...)
    str rfn = d->real_filename ();

    // must do this before trying to open the file (or anything else
    // for that matter).
    _location.set_filename (jfn);
    _ret = r;

    // Sanity check and call fopen()
    FILE *fp = open_file (rfn);
    if (!fp) {
      /* noop */
    } else if (opts & P_NOPARSE) {
      zstr z;
      if (file2zstr (fp, &z)) {
	r->set_file (file_t::raw_alloc (d, z, opts));
      }
    } else {
      yy_buffer_state *yb = yy_create_buffer (fp, ok_pub3_yy_buffer_size);
      yy_push_html_state ();
      yy_switch_to_buffer (yb);
      yyparse ();
      flex_cleanup ();
      fclose (fp);

      r->set_file (file_t::alloc (d, _out, opts));

      _out = NULL;
    }

    _ret = nullptr;
    set_current (old);

    return r->ok ();
  }

  //--------------------------------------------------------------------

  ptr<pub_parser_t> pub_parser_t::alloc () 
  { return New refcounted<pub_parser_t> (); }

  //====================================================================

  lineno_t plineno () { return parser_t::current ()->lineno (); }

  //====================================================================

  void parse_error (str s) { parser_t::current ()->error (s); }

  //========================================== parse_ret_t =============

  parse_ret_t::parse_ret_t () : _status (PARSE_OK) {}

  //--------------------------------------------------------------------

  bool 
  parse_ret_t::to_xdr (xpub_status_t *status)
  {
    if (_status == PARSE_OK) {
      status->set_status (XPUB_STATUS_OK); 
    } else {
      xpub_status_typ_t x;
      switch (_status) {
      case PARSE_ENOENT: x = XPUB_STATUS_NOENT; break;
      case PARSE_EIO:    x = XPUB_STATUS_EIO;   break;
      case PARSE_EPARSE: x = XPUB_STATUS_EPARSE; break;
      default:  panic ("unexpected parse_status type\n"); abort();
      }
      status->set_status (x);
      for (size_t i = 0; i < _errors.size (); i++) {
	status->errors->push_back (_errors[i]);
      }
    }
    return true;
  }

  //====================================================================

    ptr<zone_t> pub_str_parser_t::parse(const str &in, opts_t opts) {
        ptr<pub3::parser_t> old = current ();
        set_current (mkref (this));
        _location.set_filename ("<pub>");

        yy_parse_pub(in);
        yyparse ();
        yy_pop_all ();
        flex_cleanup ();

        ptr<zone_t> ret = _out;
        _out = nullptr;
        if (error_condition ()) {
          ret = nullptr;
        }

        set_current (old);
        return ret;
    }

} // namespace pub3

