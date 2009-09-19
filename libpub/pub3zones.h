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
  
  class zone_t {
  public:
    zone_t () {}
    virtual ~zone_t () {}
    virtual bool add (ptr<zone_t> z) { return false; }
    virtual str to_str () { return NULL; }
    virtual vec<ptr<zone_t> > *children () { return NULL; }
  };

  //-----------------------------------------------------------------------

  class zone_container_t : public zone_t {
  public:
    zone_container_t () : zone_t () {}
    vec<ptr<zone_t> > *children () { return &_children; }
  protected:
    vec<ptr<zone_t> > _children;
  };

  //-----------------------------------------------------------------------

  class zone_html_t : public zone_container_t {
  public:
    zone_html_t (bool pws) : zone_container_t (), _preserve_white_space (pws) {}
    bool add (ptr<zone_t> z);

  private:
    bool _preserve_white_space;
  };

  //-----------------------------------------------------------------------

  class zone_text_t : public zone_t {
  public:
    zone_text_t () : zone_t () {}
    bool add (ptr<zone_t> z);
    str to_str () const { return _b; }
  protected:
    strbuf _b;
    vec<str> _hold;
  };

  //-----------------------------------------------------------------------

};


#endif /* _LIBPUB_PUB3OBJ_H_ */

