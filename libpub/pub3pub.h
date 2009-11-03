// -*-c++-*-

#pragma once

#include "pub3expr.h"
#include "pub3eval.h"

namespace pub3 {

  //-----------------------------------------------------------------------
  
  class file_t;

  //-----------------------------------------------------------------------

  typedef int opts_t;
  
  // Opts can be be a bitmask of the following:
  enum {
    P_DEBUG =    0x1,     /* debug info output w/ text */
    P_IINFO =    0x2,     /* include info output w/ text */
    P_VERBOSE =  0x4,     /* debug messages, etc */
    P_VISERR =   0x8,     /* visible HTML errors */
    P_WSS =      0x10,    /* white space stripping */
    P_NOPARSE =  0x20,    /* don't parse file at all */
    P_NOLOCALE = 0x40,    /* Don't localize file */
    P_INFINITY = 0x80
  };
    
  //-----------------------------------------------------------------------

  class localizer_t : public virtual refcount {
  public:
    localizer_t () {}
    virtual ~localizer_t () {}
    virtual str localize (const str &infn) const = 0;
    virtual bool strict () const { return false; }
    virtual str get_default_fnf () const { return NULL; }
  };

  //-----------------------------------------------------------------------

  class publish_t : public eval_t {
  public:
    publish_t (ptr<bindtab_t> univerals, zbuf *z = NULL);
    publish_t (ptr<env_t> e, ptr<output_t> o) : eval_t (e, o) {}
    void publish (str nm, ptr<bind_interface_t> d, status_ev_t ev, CLOSURE);
    void publish2 (str nm, status_ev_t ev, CLOSURE);
    void set_opts (opts_t o) { _opts = o; }
    void output (str s);
    void output_err (str s);
    ptr<localizer_t> localizer ();
    void set_localizer (ptr<localizer_t> l) { _localizer = l; }
    opts_t opts () const { return _opts; }
    str set_cwd (str s) ;
    str cwd () const { return _cwd; }
    void publish (ptr<const file_t> file, status_ev_t ev);
  private:
    ptr<localizer_t> _localizer;
    vec<str> _include_stack;
    opts_t _opts;
    str _cwd;
  };

  //--------------------------------------------------------------------

  class output_std_t : public output_t {
  public:
    output_std_t (zbuf *z) : output_t (), _out (z) {}
    void output_err (location_t loc, str msg);
  private:
    zbuf *_out;
  };

  //-----------------------------------------------------------------------

  class output_silent_t : public output_t {
  public:
    output_silent_t () : output_t () {}
    void output_err (location_t loc, str msg);
  };

  //-----------------------------------------------------------------------

};
