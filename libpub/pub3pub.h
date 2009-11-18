// -*-c++-*-

#pragma once

#include "pub3base.h"
#include "pub3expr.h"
#include "pub3eval.h"

namespace pub3 {

  //-----------------------------------------------------------------------
  
  class file_t;
  class metadata_t;
  class ok_iface_t;

  //-----------------------------------------------------------------------
  
  // Opts can be be a bitmask of the following:
  enum {
    P_DEBUG =    0x1,     /* debug info output w/ text */
    P_IINFO =    0x2,     /* include info output w/ text */
    P_VERBOSE =  0x4,     /* debug messages, etc */
    P_VISERR =   0x8,     /* visible HTML errors */
    P_WSS =      0x10,    /* white space stripping */
    P_NOPARSE =  0x20,    /* don't parse file at all */
    P_NOLOCALE = 0x40,    /* Don't localize file */
    P_COPY_CONF = 0x80,   /* copy the config over to universals */
    P_INFINITY = 0x100
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

  class control_t {
  public:
    control_t () : _break (false), _continue (false) {}
    static ptr<control_t> alloc ();
    bool handle_forloop ();
    void reset_forloop ();
    bool handle_zone ();
    bool _break;
    bool _continue;
    void set_rtrn (ptr<const expr_t> x) { _return = x; }
    ptr<const expr_t> rtrn () const { return _return; }
    void set_continue (bool b) { _continue = b;  }
    void set_break (bool b) { _break = b; }
    ptr<const expr_t> _return;
  };

  //-----------------------------------------------------------------------

  class lambda_state_t {
  public:
    lambda_state_t () : _binding_stack_size (0), _overflow (false) {}
    bool is_ok () const { return !_overflow; }
    friend class publish_t;
  private:
    size_t _binding_stack_size;
    ptr<control_t> _old_control;
    bool _overflow;
  };

  //-----------------------------------------------------------------------

  class publish_t : public eval_t {
  public:
    publish_t (ptr<bindtab_t> universals, zbuf *z); // for output
    publish_t (ptr<bindtab_t> universals);          // for cfg
    publish_t (ptr<env_t> e, ptr<output_t> o);
    void publish (str nm, location_t loc,
		  ptr<bind_interface_t> d, status_ev_t ev, CLOSURE);
    void set_opts (opts_t o) { _opts = o; }
    void set_pub_iface (ptr<ok_iface_t> i) { _pub_iface = i; }
    void output (zstr s);
    void output (str s);
    void output_err (str s);
    void output_err (str s, location_t loc);
    void output_err_stacktrace (str s);
    ptr<localizer_t> localizer ();
    void set_localizer (ptr<localizer_t> l) { _localizer = l; }
    opts_t opts () const { return _opts; }
    str set_cwd (str s) ;
    str cwd () const { return _cwd; }
    void publish_file (ptr<const file_t> file, status_ev_t ev, CLOSURE);
    void push_include_location (location_t l);
    void pop_include_location ();
    void push_metadata (ptr<const metadata_t> md);
    void pop_metadata ();
    bool push_pws (bool b);
    void pop_pws (bool b);
    bool pws () const;

    // manipulate control stack

    ptr<control_t> control ();
    ptr<control_t> push_control ();
    void restore_control (ptr<control_t> c);

    lambda_state_t push_lambda_call (str fn, ptr<bindtab_t> bindings);
    ptr<const expr_t> pop_lambda_call (lambda_state_t state);

    // Keep track of where the last call was
    void update_call_location (const expr_t &x);

  private:
    ptr<localizer_t> _localizer;

    // A stack of all of the files being published, with their actual
    vec<ptr<const metadata_t> > _metadata_stack;

    // A stack of locations of file inclusions.
    vec<location_t> _include_stack;

    // A stack of all lambda calls
    vec<str> _lambda_stack;

    // Where the last call happened
    location_t _call_location;

    opts_t _opts;
    str _cwd;
    location_t _location;        // current location
    ptr<ok_iface_t> _pub_iface;  // publisher interface
    bool _pws;                   // preserve white space
    ptr<control_t> _control;     // control flow control
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
    static ptr<output_silent_t> alloc ();
  };

  //-----------------------------------------------------------------------

};
