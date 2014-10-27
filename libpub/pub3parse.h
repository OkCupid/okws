// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub3base.h"
#include "pub3expr.h"
#include "pub3ast.h"
#include "pub3file.h"
#include <stdio.h>

namespace pub3 {

  //-----------------------------------------------------------------------

  typedef enum { PARSE_OK = 0,
		 PARSE_ENOENT = 1,
		 PARSE_EIO = 2,
		 PARSE_EPARSE = 3 } parse_status_t; 

  //-----------------------------------------------------------------------

  class parser_t : public virtual refcount {
  public:
    parser_t ();
    parser_t (str f);
    lineno_t lineno () const;
    const location_t &location () const;
    location_t location (lineno_t l) const;
    void inc_lineno (lineno_t i = 1);

    static ptr<parser_t> current ();
    static void set_current (ptr<parser_t> p);

    // callbacks from bison
    virtual bool set_zone_output (ptr<zone_t> z) { return false; }
    virtual bool set_expr_output (ptr<expr_t> x) { return false; }

    // report an error;
    virtual void error (str d);
    virtual void error () { _error = true; }

    virtual const vec<str> &get_errors () const = 0;
    virtual vec<str> &get_errors () = 0 ;

  protected:
    bool error_condition () const { return get_errors ().size () || _error; }
    location_t _location;
    bool _error;
  };

  //-----------------------------------------------------------------------

  lineno_t plineno ();
  void parse_error (str e);

  //-----------------------------------------------------------------------

  class json_parser_t : public parser_t {
  public:
    json_parser_t ();
    void set_output (ptr<expr_t> e);
    ptr<expr_t> mparse (const str &in);
    static ptr<expr_t> parse (const str &in);
    bool set_expr_output (ptr<pub3::expr_t> x);
    const vec<str> &get_errors () const { return _errors; }
    vec<str> &get_errors () { return _errors; }
  protected:
    ptr<pub3::expr_t> _out;
    vec<str> _errors;
  };

  //-----------------------------------------------------------------------

  class parse_ret_t {
  public:
    parse_ret_t ();
    bool to_xdr (xpub_status_t *status);
    ptr<file_t> file () { return _file; }
    bool ok () const { return _status == PARSE_OK; }
    vec<str> &get_errors () { return _errors; }
    const vec<str> &get_errors () const { return _errors; }
    void set_status (parse_status_t s) { _status = s; }
    void set_file (ptr<file_t> f) { _file = f; }
  protected:
    parse_status_t _status;
    vec<str> _errors;
    ptr<file_t> _file;
  };

  //-----------------------------------------------------------------------

  class pub_parser_t : public parser_t {
  public:

    pub_parser_t () : parser_t () {}
    bool set_zone_output (ptr<zone_t> z);
    bool parse (ptr<metadata_t> m, parse_ret_t *out, opts_t opts = 0);
    static ptr<pub_parser_t> alloc ();

    const vec<str> &get_errors () const;
    vec<str> &get_errors ();
    void set_stauts (parse_status_t st);

    void error (str d);
    void error ();
    void error (str d, parse_status_t code);
    bool file2zstr (FILE *fp, zstr *out);

  protected:
    FILE *open_file (const str &fn);
    ptr<zone_t> _out;
    parse_ret_t *_ret;
  };

  //-----------------------------------------------------------------------

  class pub_str_parser_t : public parser_t {

    ptr<pub3::zone_t> _out;
    vec<str> _errors;

  public:
    bool set_zone_output (ptr<pub3::zone_t> z) { _out = z; return true; }
    const vec<str> &get_errors () const { return _errors; }
    vec<str> &get_errors () { return _errors; }

    ptr<pub3::zone_t> parse(const str &in, pub3::opts_t opts = 0);
  };

  //-----------------------------------------------------------------------
} // namespace pub3

