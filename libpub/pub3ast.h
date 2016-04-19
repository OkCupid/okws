// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub3expr.h"
#include "pub3env.h"
#include "pub3eval.h"
#include "okformat.h"
#include "pub3expr.h"
#include "pub3obj.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class ast_node_t : public virtual refcount, public virtual dumpable_t {
  public: 
    ast_node_t (location_t l) : _location (l) {}
    ast_node_t (lineno_t l) : _location (l) {}
    lineno_t lineno () const { return _location._lineno; }

    void publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    status_t publish_nonblock (eval_t *p) const;
    lineno_t dump_get_lineno () const { return lineno (); }

    bool might_block () const;
    virtual void propogate_metadata (ptr<const metadata_t> md);
  protected:
    virtual bool might_block_uncached () const = 0;

    location_t _location; 
    virtual status_t v_publish_nonblock (eval_t *p) const = 0;
    virtual void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    mutable tri_bool_t _might_block;
  };

  //-----------------------------------------------------------------------

  class zone_html_t;
  class zone_pub_t;
  class zone_text_t;
  class statement_t;
  class zone_raw_t;
  class zone_wss_boundary_t;
  
  class zone_t : public ast_node_t {
  public:
    zone_t (location_t l) : ast_node_t (l) {}
    zone_t (lineno_t l) : ast_node_t (l) {}
    virtual ~zone_t () {}
    virtual str to_str () { return NULL; }
    virtual vec<ptr<zone_t> > *children () { return NULL; }
    virtual ptr<zone_html_t> zone_html () { return NULL; }
    virtual zone_pub_t *zone_pub () { return NULL; }
    virtual ptr<zone_text_t> zone_text () { return NULL; }
    virtual bool to_xdr (xpub3_zone_t *z) const = 0;

    static ptr<zone_t> alloc (const xpub3_zone_t &z);
    static ptr<zone_t> alloc (const xpub3_zone_t *z);
  protected:
    bool handle_control (eval_t *p) const;
  };

  //-----------------------------------------------------------------------

  typedef event<ptr<const zone_t> >::ref czev_t;

  //-----------------------------------------------------------------------

  class zone_container_t : public zone_t {
  public:
    zone_container_t (location_t l) : zone_t (l) {}
    zone_container_t (lineno_t l) : zone_t (l) {}
    vec<ptr<zone_t> > *children () { return &_children; }
    virtual void propogate_metadata (ptr<const metadata_t> md);
  protected:
    vec<ptr<zone_t> > _children;
  };

  //-----------------------------------------------------------------------

  class zone_raw_t : public zone_t {
  public:
    zone_raw_t (location_t l, zstr s);
    zone_raw_t (const xpub3_zone_raw_t &r);
    static ptr<zone_raw_t> alloc (location_t l, zstr s);
    static ptr<zone_raw_t> alloc (const xpub3_zone_raw_t &x);
    bool to_xdr (xpub3_zone_t *z) const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "zone_raw_t"; }
  protected:
    bool might_block_uncached () const;
    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
  private:
    zstr _data;
  };

  //-----------------------------------------------------------------------

  class zone_text_t : public zone_t {
  public:
    zone_text_t (location_t l) : zone_t (l) {}
    zone_text_t (const xpub3_zone_text_t &z);

    static ptr<zone_text_t> alloc ();
    static ptr<zone_text_t> alloc (str s);
    static ptr<zone_text_t> alloc (char c);
    static ptr<zone_text_t> alloc (const xpub3_zone_text_t &z);

    bool add (ptr<zone_t> z);
    str to_str () const { return _b; }
    void add (str s);
    void add (char c);
    ptr<zone_text_t> zone_text () { return mkref (this); }

    bool to_xdr (xpub3_zone_t *z) const;
    bool might_block_uncached () const { return false; }
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "zone_text_t"; }
  protected:
    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    void cook_text () const;

    // while parsing, use the following representation:
    strbuf _b;
    vec<str> _hold;

    // otherwise, here are the buffers to use:
    mutable zstr _original;
    mutable zstr _wss;
  };

  //-----------------------------------------------------------------------

  class zone_wss_boundary_t : public zone_t {
  public:
    zone_wss_boundary_t (location_t l, bool on, str tag) 
      : zone_t (l), _on (on), _tag (tag) {}
    zone_wss_boundary_t (const xpub3_zone_wss_boundary_t &x);

    static ptr<zone_wss_boundary_t> alloc (const xpub3_zone_wss_boundary_t &z);
    static ptr<zone_wss_boundary_t> alloc (bool on, str tag);

    bool to_xdr (xpub3_zone_t *z) const;
    bool might_block_uncached () const { return false; }
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "zone_text_t"; }
  protected:
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    status_t v_publish_nonblock (eval_t *p) const;
    bool _on;
    str _tag;
  };

  //-----------------------------------------------------------------------

  class zone_html_t : public zone_container_t {
  public:
    zone_html_t (location_t l);
    zone_html_t (lineno_t l);
    zone_html_t (const xpub3_zone_html_t &z);
    void add (ptr<zone_t> z);
    void add (str s);
    void add (char ch);
    void add_boundary (str s);
    ptr<zone_html_t> zone_html () { return mkref (this); }
    static ptr<zone_html_t> alloc (ptr<zone_t> z);
    static ptr<zone_html_t> alloc ();
    static ptr<zone_html_t> alloc (const xpub3_zone_html_t &x);
    void unshift (str s);
    bool to_xdr (xpub3_zone_t *z) const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "zone_html_t"; }

  protected:
    bool might_block_uncached () const;
    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    ptr<zone_text_t> push_zone_text ();
  };

  //-----------------------------------------------------------------------

  class zone_inline_expr_t : public zone_t {
  public:
    zone_inline_expr_t (location_t l, ptr<expr_t> e);
    zone_inline_expr_t (const xpub3_zone_inline_expr_t &z);
    static ptr<zone_inline_expr_t> alloc (ptr<expr_t> e);
    static ptr<zone_inline_expr_t> alloc (const xpub3_zone_inline_expr_t &x);
    bool to_xdr (xpub3_zone_t *z) const;

    bool might_block_uncached () const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "zone_inline_expr_t"; }
    virtual void propogate_metadata (ptr<const metadata_t> md);
  protected:
    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    void null_warn (eval_t *p) const;
    ptr<expr_t> _expr;
  };

  //-----------------------------------------------------------------------

  class zone_pub_t : public zone_t {
  public:
    zone_pub_t (location_t l);
    zone_pub_t (const xpub3_zone_pub_t &z);
    static ptr<zone_pub_t> alloc ();

    struct pair_t {
      ptr<statement_t> first;
      ptr<statement_t> second;
    };
    static ptr<zone_pub_t> alloc (const xpub3_zone_pub_t &x);

    // reserve one spot extra, and get rid of it if not required
    void reserve ();
    void unreserve ();
    void take_reserved_slot (ptr<statement_t> s);
    void add (ptr<statement_t> s);
    void add (pair_t zp);
    bool add (ptr<zone_t> z);
    vec<ptr<statement_t> > *statements () { return &_statements; }
    zone_pub_t *zone_pub () { return this; }
    bool to_xdr (xpub3_zone_t *z) const;
    bool might_block_uncached () const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "zone_pub_t"; }
    virtual void propogate_metadata (ptr<const metadata_t> md);
    
  protected:
    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    vec<ptr<statement_t> > _statements;
  };

  //-----------------------------------------------------------------------

  class statement_t : public ast_node_t {
  public:
    statement_t (location_t l) : ast_node_t (l) {}
    statement_t (lineno_t l) : ast_node_t (l) {}
    virtual bool to_xdr (xpub3_statement_t *x) const = 0;
    static ptr<statement_t> alloc (const xpub3_statement_t &x);
  };

  //-----------------------------------------------------------------------

  // usful for {{ html }} zones inside of {% pub areas %}
  class statement_zone_t : public statement_t {
  public:
    statement_zone_t (location_t l, ptr<zone_t> z);
    statement_zone_t (const xpub3_statement_zone_t &z);
    static ptr<statement_zone_t> alloc (ptr<zone_t> z);
    bool to_xdr (xpub3_statement_t *x) const;

    bool might_block_uncached () const;
    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "statement_zone_t"; }
    virtual void propogate_metadata (ptr<const metadata_t> md);
  protected:
    ptr<zone_t> _zone;
  };

  //-----------------------------------------------------------------------

  class expr_statement_t : public statement_t {
  public:
    expr_statement_t (location_t l) : statement_t (l) {}
    expr_statement_t (const xpub3_expr_statement_t &x);
    expr_statement_t (ptr<expr_t> x, location_t l);
    static ptr<expr_statement_t> alloc (ptr<expr_t> x);
    bool to_xdr (xpub3_statement_t *x) const;
    void add (ptr<expr_t> x);

    bool might_block_uncached () const;
    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "expr_statement_t"; }
    virtual void propogate_metadata (ptr<const metadata_t> md);
  protected:
    ptr<expr_t> _expr;
  };

  //-----------------------------------------------------------------------

  class while_t : public statement_t {
  public:
    while_t (location_t l) : statement_t (l) {}
    while_t (lineno_t l) : statement_t (l) {}
    while_t (const xpub3_while_t &x);
    bool to_xdr (xpub3_statement_t *x) const;

    static ptr<while_t> alloc (lineno_t l);

    bool add_cond (ptr<expr_t> c);
    bool add_body (ptr<zone_t> z);

    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    status_t v_publish_nonblock (eval_t *p) const;
    bool might_block_uncached () const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "while_t"; }
    virtual void propogate_metadata (ptr<const metadata_t> md);
  protected:
    ptr<expr_t> _cond;
    ptr<zone_t> _body;

  private:
    bool handle_control (eval_t *p) const;
    void reset_control (eval_t *p) const;
  };

  //-----------------------------------------------------------------------

  class for_t : public statement_t {
  public:
    for_t (location_t l) : statement_t (l) {}
    for_t (lineno_t l) : statement_t (l) {}
    for_t (const xpub3_for_t &x);
    bool to_xdr (xpub3_statement_t *x) const;

    static ptr<for_t> alloc (lineno_t l);

    bool add_params (ptr<expr_list_t> l);
    bool add_body (ptr<zone_t> z);
    bool add_empty (ptr<zone_t> z);

    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    status_t v_publish_nonblock (eval_t *p) const;
    bool might_block_uncached () const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "for_t"; }
    virtual void propogate_metadata (ptr<const metadata_t> md);
  protected:
    ptr<expr_list_t> eval_list (eval_t *p) const;
    void pub_list (eval_t *p, xlev_t ev, CLOSURE) const;
    str _iter;
    ptr<expr_t> _arr;
    ptr<zone_t> _body;
    ptr<zone_t> _empty;

  private:
    void err_empty (eval_t *pub) const;
    void err_badrow (eval_t *pub, size_t i) const;
    bool handle_control (eval_t *p) const;
    void reset_control (eval_t *p) const;
  };

  //-----------------------------------------------------------------------

  class if_clause_t : virtual public dumpable_t {
  public:
    if_clause_t (lineno_t l) : _lineno (l) {}
    if_clause_t (const xpub3_if_clause_t &x);

    static ptr<if_clause_t> alloc ();

    void add_expr (ptr<expr_t> e) { _expr = e; }
    void add_body (ptr<zone_t> e) { _body = e; }

    bool to_xdr (xpub3_if_clause_t *x) const;
    bool to_xdr (xpub3_statement_t *x) const { return false; }

    ptr<const expr_t> expr () const { return _expr; }
    ptr<const zone_t> body () const { return _body; }
    bool might_block () const;
    bool fits (eval_t *p) const;
    void fits (eval_t *p, evb_t ev, CLOSURE) const;

    const char *get_obj_name () const { return "if_clause_t"; }
    lineno_t dump_get_lineno () const { return _lineno; }
    void v_dump (dumper_t *d) const;
    virtual void propogate_metadata (ptr<const metadata_t> md);

  private:
    lineno_t _lineno;
    ptr<expr_t> _expr;
    ptr<zone_t> _body;
    mutable tri_bool_t _might_block;
  };

  //-----------------------------------------------------------------------

  typedef vec<ptr<if_clause_t> > if_clause_list_t;

//-----------------------------------------------------------------------------

    class identifier_list_t {
    public:
    
        identifier_list_t() { }

        void push_back(str s, bool opt = false);
        bool check_args(ptr<const expr_list_t> a);
        size_t size() const;
        size_t opt_size() const;
        size_t max_size() const;

        identifier_list_t& operator+= (const identifier_list_t& l);
        str operator[] (size_t index) const;

    private:

        vec<str> _params;
        vec<str> _opt_params;
    };

  //-----------------------------------------------------------------------
  
  class if_t : public statement_t {
  public:
    if_t (location_t l) : statement_t (l) {}
    if_t (const xpub3_if_t &x);

    static ptr<if_t> alloc (lineno_t l);

    void add_clauses (ptr<if_clause_list_t> c);
    void add_clause (ptr<if_clause_t> c);

    bool to_xdr (xpub3_statement_t *x) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    status_t v_publish_nonblock (eval_t *p) const;
    bool might_block_uncached () const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "if_t"; }
    virtual void propogate_metadata (ptr<const metadata_t> md);
  private:
    ptr<const zone_t> find_clause (eval_t *p) const;
    void find_clause (eval_t *p, czev_t ev, CLOSURE) const;
    ptr<if_clause_list_t> _clauses;
  };

  //-----------------------------------------------------------------------

  class decl_block_t : public statement_t {
  public:
    decl_block_t (const xpub3_decls_t &x);
    decl_block_t (location_t l) : statement_t (l) {}
    bool to_xdr (xpub3_statement_t *x) const;
    virtual xpub3_statement_typ_t statement_typ () const = 0;
    void add (ptr<bindlist_t> l);
    bool is_static () const;
    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    virtual env_t::layer_type_t get_decl_type () const = 0;
    bool might_block_uncached () const;
    void v_dump (dumper_t *d) const;
    virtual void propogate_metadata (ptr<const metadata_t> md);
  protected:
    ptr<bindlist_t> _bindings;
    ptr<expr_dict_t> _tab;
    mutable tri_bool_t _static;
  };

  //-----------------------------------------------------------------------

  class locals_t : public decl_block_t {
  public:
    locals_t (const xpub3_decls_t &x) : decl_block_t (x) {}
    locals_t (location_t l) : decl_block_t (l) {}
    static ptr<locals_t> alloc (lineno_t l);
    xpub3_statement_typ_t statement_typ () const 
    { return XPUB3_STATEMENT_LOCALS; }
    env_t::layer_type_t get_decl_type () const { return env_t::LAYER_LOCALS; }
    const char *get_obj_name () const { return "locals_t"; }
  };

  //-----------------------------------------------------------------------

  class globals_t : public decl_block_t {
  public:
    globals_t (const xpub3_decls_t &x) : decl_block_t (x) {}
    globals_t (location_t l) : decl_block_t (l) {}
    static ptr<globals_t> alloc (lineno_t l);
    xpub3_statement_typ_t statement_typ () const 
    { return XPUB3_STATEMENT_GLOBALS; }

    env_t::layer_type_t get_decl_type () const 
    { return env_t::LAYER_GLOBALS; }
    const char *get_obj_name () const { return "globals_t"; }
  };

  //-----------------------------------------------------------------------

  class universals_t : public decl_block_t {
  public:
    universals_t (const xpub3_decls_t &x) : decl_block_t (x) {}
    universals_t (location_t l) : decl_block_t (l) {}
    static ptr<universals_t> alloc (lineno_t l);
    xpub3_statement_typ_t statement_typ () const 
    { return XPUB3_STATEMENT_UNIVERSALS; }

    env_t::layer_type_t get_decl_type () const 
    { return env_t::LAYER_UNIVERSALS; }
    const char *get_obj_name () const { return "universals_t"; }
  };

  //-----------------------------------------------------------------------

  class case_t : virtual public dumpable_t {
  public:
    case_t (lineno_t l) : _lineno (l) {}
    case_t (const xpub3_case_t &x);
    static ptr<case_t> alloc ();
    static ptr<case_t> alloc (const xpub3_case_t *x);
    void add_key (ptr<expr_t> x);
    void add_zone (ptr<zone_t> z);
    bool to_xdr (xpub3_case_t *x) const;
    ptr<zone_t> zone () { return  _zone; }
    ptr<const zone_t> zone () const { return _zone; }
    ptr<const expr_t> key () const { return _key; }
    bool might_block () const;
    lineno_t dump_get_lineno () const { return _lineno; }
    const char *get_obj_name () const { return "case_t"; }
    void v_dump (dumper_t *d) const;
    virtual void propogate_metadata (ptr<const metadata_t> md);
  protected:
    lineno_t _lineno;
    ptr<expr_t> _key;
    ptr<zone_t> _zone;
  };

  //-----------------------------------------------------------------------

  class case_list_t : public vec<ptr<case_t > > {
  public:
    case_list_t () {}
    case_list_t (const xpub3_cases_t &x);
    static ptr<case_list_t> alloc ();
    static ptr<case_list_t> alloc (const xpub3_cases_t &x);
    void add_case (ptr<case_t> c);
    bool to_xdr (xpub3_cases_t *x) const;
    void propogate_metadata (ptr<const metadata_t> md);
  };

  //-----------------------------------------------------------------------

  class switch_t : public statement_t {
  public:
    switch_t (location_t l) : statement_t (l) {}
    switch_t (const xpub3_switch_t &x);
    static ptr<switch_t> alloc ();
    bool add_cases (ptr<case_list_t> l);
    void add_key (ptr<expr_t> x);
    bool to_xdr (xpub3_statement_t *x) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    status_t v_publish_nonblock (eval_t *p) const;
    bool might_block_uncached () const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "switch_t"; }
    virtual void propogate_metadata (ptr<const metadata_t> md);
  protected:
    ptr<const zone_t> find_case (eval_t *pub) const;
    bool populate_cases ();
    ptr<expr_t> _key;
    ptr<case_list_t> _cases;
    qhash<str, ptr<case_t> > _map;
    ptr<case_t> _default, _null;
  };

  //-----------------------------------------------------------------------

  class include_t : public statement_t {
  public:
    include_t (location_t l) : statement_t (l) {}
    include_t (const xpub3_include_t &x);
    static ptr<include_t> alloc ();
    bool add_args (ptr<expr_list_t> l, str *errp);
    virtual str fnname () const { return "include"; }
    bool to_xdr (xpub3_statement_t *x) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    status_t v_publish_nonblock (eval_t *p) const;
    bool might_block_uncached () const { return true; }
    virtual bool muzzle_output () const { return false; }
    void v_dump (dumper_t *d) const;
    virtual const char *get_obj_name () const { return "include_t"; }
    virtual void propogate_metadata (ptr<const metadata_t> md);
  protected:
    bool to_xdr_base (xpub3_statement_t *x, xpub3_statement_typ_t typ) const;
    ptr<expr_t> _file;
    ptr<expr_t> _dict;
  };

  //-----------------------------------------------------------------------

  class load_t : public include_t {
  public:
    load_t (location_t l) : include_t (l) {}
    load_t (const xpub3_include_t &x);
    bool to_xdr (xpub3_statement_t *x) const;
    bool muzzle_output () const { return true; }
    str fnname () const { return "load"; }
    static ptr<load_t> alloc ();
    const char *get_obj_name () const { return "load_t"; }
  };

  //-----------------------------------------------------------------------

  class print_t : public statement_t {
  public:
    print_t (location_t l) : statement_t (l) {}
    print_t (const xpub3_print_t &x);
    static ptr<print_t> alloc (lineno_t l);
    
    bool add (ptr<pub3::expr_list_t> l);
    bool to_xdr (xpub3_statement_t *x) const;

    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    bool might_block_uncached () const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "print_t"; }
    virtual void propogate_metadata (ptr<const metadata_t> md);

  private:
    ptr<pub3::expr_list_t> _args;
  };

  //-----------------------------------------------------------------------

  class break_t : public statement_t {
  public:
    break_t (location_t l) : statement_t (l) {}
    break_t (const xpub3_break_t &x);
    static ptr<break_t> alloc ();
    bool to_xdr (xpub3_statement_t *x) const;

    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    bool might_block_uncached () const { return false; }
    const char *get_obj_name () const { return "break_t"; }
  };

  //-----------------------------------------------------------------------

  class return_t : public statement_t {
  public:
    return_t (location_t l, ptr<expr_t> x) : statement_t (l), _val (x) {}
    return_t (const xpub3_return_t &x);
    static ptr<return_t> alloc (ptr<expr_t> x);
    bool to_xdr (xpub3_statement_t *x) const;

    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    bool might_block_uncached () const;
    void v_dump (dumper_t *d) const;
    const char *get_obj_name () const { return "return_t"; }
    virtual void propogate_metadata (ptr<const metadata_t> md);
  private:
    ptr<expr_t> _val;
  };

  //-----------------------------------------------------------------------

  class exit_t : public statement_t {
  public:
    exit_t (location_t l) : statement_t (l) {}
    exit_t (const xpub3_exit_t &x);
    static ptr<exit_t> alloc ();
    bool to_xdr (xpub3_statement_t *x) const;

    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    bool might_block_uncached () const { return false; }
    const char *get_obj_name () const { return "exit_t"; }
  };

  //-----------------------------------------------------------------------

  class continue_t : public statement_t {
  public:
    continue_t (location_t l) : statement_t (l) {}
    continue_t (const xpub3_continue_t &x);
    static ptr<continue_t> alloc ();
    bool to_xdr (xpub3_statement_t *x) const;

    status_t v_publish_nonblock (eval_t *p) const;
    void v_publish (eval_t *p, status_ev_t ev, CLOSURE) const;
    bool might_block_uncached () const { return false; }
    const char *get_obj_name () const { return "continue_t"; }
  };

  //-----------------------------------------------------------------------

};

