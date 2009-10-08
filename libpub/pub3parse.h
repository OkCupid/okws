// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub3base.h"
#include "pub3expr.h"
#include "pub3ast.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class parser_t {
  public:
    parser_t (str f);
    lineno_t lineno () const;
    const location_t &location () const;
    void inc_lineno (lineno_t i = 1);

    static ptr<parser_t> current ();
    static void set_current (ptr<parser_t> p);

    // callbacks from bison
    virtual bool set_zone_output (ptr<pub3::zone_t> z) { return false; }
    virtual bool set_expr_output (ptr<pub3::expr_t> x) { return false; }

  protected:
    location_t _location;
  };

  //-----------------------------------------------------------------------

  class json_parser_t : public parser_t {
  public:
    json_parser_t ();
    void set_output (ptr<expr_t> e);
    ptr<expr_t> parse (const str &in);
    bool set_expr_output (ptr<pub3::expr_t> x);
  protected:
    ptr<pub3::expr_t> _out;
  };

  //-----------------------------------------------------------------------

  class pub_parser_t : public parser_t {
  public:
    pub_parser_t (str f) : parser_t (f) {}
    void set_zone_output (ptr<zone_t> z);
    ptr<file_t> parse (ptr<metadata_t> m);
    bool set_zone_output (ptr<pub3::zone_t> z);
    const vec<str> &errors () const;
    void error (str d);
  protected:
    FILE *open_file (const str &fn);
    location_t _location;
    ptr<zone_t> _out;
    vec<str> _errors;
  };

  //-----------------------------------------------------------------------

};

