// -*-c++-*-

#pragma once

#include "async.h"
#include "qhash.h"
#include "pub3base.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class dumper_t {
  public:
    dumper_t ();
    void begin_obj (str typ, void *p, lineno_t l);
    void end_obj ();
    void dump (str s, bool nl = true);
    enum { INDENT = 4 };
    str output () const { return _buf; }
  private:
    void setspace ();
    void inc_level ();
    void dec_level ();
    size_t _level;
    vec<str> _hold;
    strbuf _buf;
    str _space;
    qhash<size_t,str> _spaces;
  };
  
  //-----------------------------------------------------------------------

  class dumpable_t {
  public:
    dumpable_t () {}
    virtual ~dumpable_t () {}
    virtual void dump (dumper_t *d) const {}
    virtual const char *get_obj_name () const = 0;
    virtual lineno_t dump_get_lineno () const { return 0; }
  };

  //-----------------------------------------------------------------------

}
