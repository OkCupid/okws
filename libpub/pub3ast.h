// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub3expr.h"
#include "okformat.h"
#include "pub3expr.h"
#include "pub3obj.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class ast_node_t : public virtual refcount {
  public: 
    ast_node_t (location_t l) : _location (l) {}
  protected:
    location_t _location; 
  };

  //-----------------------------------------------------------------------

  class zone_html_t;
  class zone_pub_t;
  
  class zone_t : public ast_node_t {
  public:
    zone_t (location_t l) : ast_node_t (l) {}
    virtual ~zone_t () {}
    virtual str to_str () { return NULL; }
    virtual vec<ptr<zone_t> > *children () { return NULL; }
    virtual ptr<zone_html_t> zone_html () { return NULL; }
    virtual zone_pub_t *zone_pub () { return NULL; }
    virtual ptr<zone_text_t> zone_text () { return NULL; }
  };

  //-----------------------------------------------------------------------

  class zone_container_t : public zone_t {
  public:
    zone_container_t (location_t l) : zone_t (l) {}
    vec<ptr<zone_t> > *children () { return &_children; }
  protected:
    vec<ptr<zone_t> > _children;
  };

  //-----------------------------------------------------------------------

  class zone_html_t : public zone_container_t {
  public:
    zone_html_t (location_t l);
    void add (ptr<zone_t> z);
    void add (str s);
    void add (char ch);
    ptr<zone_html_t> zone_html () { return mkref (this); }
    static ptr<zone_html_t> alloc (ptr<zone_t> z);
    bool preserve_white_space () const { return _preserve_white_space; }
    void set_preserve_white_space (bool b) { _preserve_white_space = b; }
  protected:
    ptr<zone_text_t> push_zone_text ();
  private:
    bool _preserve_white_space;
  };

  //-----------------------------------------------------------------------

  class zone_text_t : public zone_t {
  public:
    zone_text_t (location_t l) : zone_t (l) {}

    static ptr<zone_text_t> alloc ();
    static ptr<zone_text_t> alloc (str s);
    static ptr<zone_text_t> alloc (char c);

    bool add (ptr<zone_t> z);
    str to_str () const { return _b; }
    void add (str s);
    void add (char c);
    ptr<zone_text_t> zone_text () { return this; }
  protected:

    // while parsing, use the following representation:
    strbuf _b;
    vec<str> _hold;

    // otherwise, here are the buffers to use:
    zstr _original;
    zstr _wss;
  };

  //-----------------------------------------------------------------------

  class zone_inline_expr_t : public zone_t {
  public:
    zone_inline_expr_t (location_t l, ptr<expr_t> e);
    static ptr<zone_inline_expr_t> alloc (ptr<expr_t> e);
  protected:
    ptr<expr_t> _expr;
  };

  //-----------------------------------------------------------------------

  class zone_pub_t : public zone_t {
  public:
    zone_pub_t (location_t l);
    static ptr<zone_pub_t> alloc (ptr<expr_t> e);

    struct pair_t {
      ptr<statement_t> first;
      ptr<statement_t> second;
    };

    // reserve one spot extra, and get rid of it if not required
    void reserve ();
    void unreserve ();
    void take_reserved_slot (ptr<statement_t> s);
    void add (ptr<statement_t> s);
    void add (zone_pair_t zp);
    bool add (ptr<zone_t> z);
    vec<ptr<statement_t> > *statements () { return &_statements; }
    zone_pub_t *zone_pub () { return this; }
    
  protected:
    vec<ptr<statement_t> > _statements;
  };

  //-----------------------------------------------------------------------

  class statement_t : public ast_node_t {
  public:
    statement_t (location_t l) : ast_node_t (l) {}
  };

  //-----------------------------------------------------------------------

  class statement_zone_t : public statement_t {
  public:
    statement_zone_t (location_t l, ptr<zone_t> z);
    static ptr<statement_zone_t> alloc (ptr<zone_t> z);
  protected:
    location_t _location;
    ptr<zone_t> _zone;
  };

  //-----------------------------------------------------------------------

  class expr_statement_t : public statement_t {
  public:
    expr_statement_t (location l) : statement_t (l) {}
    static ptr<expr_statement_t> alloc (ptr<expr_t> x);
    void add (ptr<expr_t> x);
  protected:
    ptr<expr_t> _expr;
  };

  //-----------------------------------------------------------------------

  class for_t : public statement_t {
  public:
    for_t (location_t l) : statement_t (l) {}
    for_t (const xpub3_for_t &x);
    bool to_xdr (xpub_obj_t *x) const;

    static ptr<for_t> alloc ();

    bool add_params (ptr<expr_list_t> l);
    bool add_body (ptr<zone_t> z);
    bool add_empty (ptr<zone_t> z);

    const char *get_obj_name () const { return "pub3::for_t"; }
    virtual void publish (pub2_iface_t *, output_t *, penv_t *, 
			  xpub_status_cb_t , CLOSURE) const;
    bool publish_nonblock (pub2_iface_t *, output_t *, penv_t *) const;
    void output (output_t *o, penv_t *e) const;
    bool might_block () const { return true; }
  protected:
    str _iter;
    ptr<expr_t> _arr;
    ptr<zone_t> _body;
    ptr<zone_t> _empty;
  };

  //-----------------------------------------------------------------------

  class if_clause_t : public statement_t {
  public:
    if_clause_t (location_t l) : statement_t (l) {}
    if_clause_t (const xpub3_if_clause_t &x);

    static ptr<if_clause_t> alloc ();

    void add_expr (ptr<expr_t> e) { _expr = e; }
    void add_body (ptr<zone_t> e) { _body = e; }

    bool to_xdr (xpub3_if_clause_t *x) const;

    ptr<const expr_t> expr () const { return _expr; }
    ptr<nested_env_t> env () const { return _env; }
    bool might_block () const;

  private:
    int _lineno;
    ptr<expr_t> _expr;
    ptr<zone_t> _body;
  };

  //-----------------------------------------------------------------------

  typedef vec<ptr<if_clause_t> > if_clause_list_t;
  typedef vec<str> identifier_list_t;

  //-----------------------------------------------------------------------
  
  class if_t : public statement_t {
  public:
    if_t (location_t l) : statement_t (l), _might_block (-1) {}
    if_t (const xpub3_if_t &x);

    static ptr<if_t> alloc ();

    void add_clauses (ptr<if_clause_list_t> c);
    void add_clause (ptr<if_clause_t> c);

    const char *get_obj_name () const { return "pub3::if_t"; }
    bool to_xdr (xpub_obj_t *x) const;
    void publish (pub2_iface_t *, output_t *, penv_t *, 
		  xpub_status_cb_t , CLOSURE) const;
    bool publish_nonblock (pub2_iface_t *, output_t *, penv_t *) const;
    void output (output_t *o, penv_t *e) const;
    bool might_block () const;

  private:
    ptr<zone_t> find_clause (output_t *o, penv_t *e) const;

    ptr<if_clause_list_t> _clauses;
    mutable int _might_block; // one of: -1 (not set), 0 (no), and 1 (yes)
  };

  //-----------------------------------------------------------------------

  class locals_t : public statement_t {
  public:
    locals_t (location_t l) : statement_t (l) {}
    static ptr<locals_t> alloc ();
  protected:
    ptr<bindtab_t> _bindings;
  };

  //-----------------------------------------------------------------------

  class universals_t : public statement_t {
  public:
    universals_t (location_t l) : statement_t (l) {}
    static ptr<universals_t> alloc ();
  protected:
    ptr<bindtab_t> _bindings;
  };

  //-----------------------------------------------------------------------

  class case_t : public statement_t {
  public:
    case_t (location_t l) : statement_t (l) {}
    static ptr<case_t> alloc ();
    void add_key (const str &k);
    void add_zone (ptr<zone_t> z);
  protected:
    str _key;
    ptr<zone_t> _zone;
  };

  //-----------------------------------------------------------------------

  class case_list_t : public vec<ptr<case_t > > {
  public:
    case_list_t (location_t l) : statement_t (l) {}
    static ptr<case_list_t> alloc ();
    void add_case (ptr<case_t> c);
  };

  //-----------------------------------------------------------------------

  class switch_t : public statement_t {
  public:
    switch_t (location_t l) : statement_t (l) {}
    static ptr<switch_t> alloc ();
    void add_cases (ptr<case_list_t> l);
    void add_key (ptr<expr_t> x);
  protected:
    ptr<expr_t> _key;
    ptr<vec<case_list_t> > _cases;
    qhash<str, ptr<case_t> > _map;
    ptr<case_t> _default;
  };

  //-----------------------------------------------------------------------

  class include_t : public statement_t {
  public:
    include_t (location_t l) : statement_t (l) {}
    include_t (const xpub3_include_t &x);
    bool might_block () const { return true; }
    static ptr<include_t> alloc ();
    bool add_args (ptr<expr_list_t> l, str *errp);
    virtual str fnname () const { return "include"; }
  protected:
    ptr<expr_t> _file;
    ptr<expr_t> _dict;
  };

  //-----------------------------------------------------------------------

  class load_t : public include_t {
  public:
    load_t (location_t l) : include_t (l) {}
    load_t (const xpub3_include_t &x);
    bool to_xdr (xpub_obj_t *x) const;
    bool muzzle_output () const { return true; }
    str fnname () const { return "load"; }
  };

  //-----------------------------------------------------------------------

  class print_t : public statement_t {
  public:
    print_t (location_t l) : statement_t (l) {}
    print_t (const xpub3_print_t &x);
    
    bool add (ptr<pub3::expr_list_t> l);
    bool to_xdr (xpub_obj_t *x) const;
    bool publish_nonblock (pub2_iface_t *, output_t *, penv_t *) const;
    void output (output_t *o, penv_t *e) const;

  private:
    ptr<pub3::expr_list_t> _args;
  };

  //-----------------------------------------------------------------------

  class fndef_t : public statement_t {
  public:
    fndef_t (str nm, location_t l) : statement_t (l), _name (nm) {}
    fndef_t (const xpub3_fndef_t &x) {}
    static ptr<fndef_t> alloc (str nm);
    void add_params (ptr<identifier_list_t> p);
    void add_body (ptr<zone_t> z);
  protected:
    str _name;
    ptr<identifier_list_t> _params;
    ptr<zone_t> _body;
  };

  //-----------------------------------------------------------------------

};

