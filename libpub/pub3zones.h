// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#ifndef _LIBPUB_PUB3PARSE_H_
#define _LIBPUB_PUB3PARSE_H_

#include "pub.h"
#include "parr.h"
#include "pub3expr.h"
#include "okformat.h"
#include "pub3expr.h"
#include "pub3obj.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class zone_html_t;
  
  class zone_t {
  public:
    zone_t (location_t l) : _location (l) {}
    virtual ~zone_t () {}
    virtual bool add (ptr<zone_t> z) { return false; }
    virtual str to_str () { return NULL; }
    virtual vec<ptr<zone_t> > *children () { return NULL; }
    virtual zone_html_t *zone_html () { return NULL; }

    location_t _location; 
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
      ptr<expr_statement_t> first;
      ptr<expr_statement_t> second;
    };

    // reserve one spot extra, and get rid of it if not required
    void reserve ();
    void unreserve ();
    void take_reserved_slot (ptr<expr_statement_t> s);
    void add (ptr<expr_statement_t> s);
    void add (zone_pair_t zp);
    
  protected:
    vec<ptr<expr_statement_t> > _statements;
  };

  //-----------------------------------------------------------------------

};


#endif /* _LIBPUB_PUB3OBJ_H_ */

