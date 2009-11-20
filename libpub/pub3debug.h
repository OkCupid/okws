// -*-c++-*-

#pragma once

#include "async.h"
#include "qhash.h"
#include "pub3base.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class dumpable_t;

  //-----------------------------------------------------------------------

  class dumper_t {
  public:
    dumper_t ();
    void begin_obj (str typ, const void *p, lineno_t l);
    void end_obj ();
    void dump (str s, bool nl = true);
    enum { INDENT = 4 };
    void dump_to (strbuf &b);
    static void dump_to_stderr (const dumpable_t *d);

    struct line_t {
      line_t (size_t l) : _ilev (l) {}
      str to_str () const { return (_s = _b); }
      void append (str s);
      size_t level () const { return _ilev; }
    private: 
      size_t _ilev;
      vec<str> _v;
      strbuf _b;
      mutable str _s;
    };

    str get_space (size_t l);
  private:
    ptr<line_t> get_curr ();
    void push_curr ();

    void setspace ();
    void inc_level ();
    void dec_level ();

    size_t _level;
    vec<ptr<line_t> > _lines;
    ptr<line_t> _curr;
    
    qhash<size_t,str> _spaces;
    bool _need_space;
  };
  
  //-----------------------------------------------------------------------

  class dumpable_t {
  public:
    dumpable_t () {}
    virtual ~dumpable_t () {}
    void dump (dumper_t *d) const;
    static void s_dump (dumper_t *d, str prfx, const dumpable_t *obj);
    virtual void v_dump (dumper_t *d) const {}
    virtual const char *get_obj_name () const = 0;
    virtual lineno_t dump_get_lineno () const { return 0; }
  };

  //-----------------------------------------------------------------------

}
