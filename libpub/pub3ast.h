// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub.h"
#include "parr.h"
#include "pub3expr.h"
#include "okformat.h"
#include "pub3expr.h"
#include "pub3obj.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class ast_node_t {
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
    virtual bool add (ptr<zone_t> z) { return false; }
    virtual str to_str () { return NULL; }
    virtual vec<ptr<zone_t> > *children () { return NULL; }
    virtual zone_html_t *zone_html () { return NULL; }
    virtual zone_pub_t *zone_pub () { return NULL; }
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
    zone_html_t (location_t l, bool pws);
    bool add (ptr<zone_t> z);
    zone_html_t *zone_html () { return this; }
    static ptr<zone_html_t> alloc (int pws);
    bool preserve_white_space () const { return _preserve_white_space; }

  private:
    bool _preserve_white_space;
  };

  //-----------------------------------------------------------------------

  class zone_text_t : public zone_t {
  public:
    zone_text_t (location_t l) : zone_t (l) {}
    static ptr<zone_text_t> alloc ();
    bool add (ptr<zone_t> z);
    str to_str () const { return _b; }
  protected:
    strbuf _b;
    vec<str> _hold;
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
    ptr<expr_dict_t> _bindings;
  };

  //-----------------------------------------------------------------------

  class universals_t : public statement_t {
  public:
    universals_t (location_t l) : statement_t (l) {}
    static ptr<universals_t> alloc ();
  protected:
    ptr<expr_dict_t> _bindings;
  };

  //-----------------------------------------------------------------------

};

