// -*-c++-*-
/* $Id$ */

#pragma once

#include "pub3expr.h"
#include "pub3debug.h"
#include "pub3env.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  // Forward-declared; more information available in pub3out.h
  class output_t;
  
  // Forward-declared class available in pub3file.h
  class metadata_t;
  class file_t;

  // Forward-declared classes available in pub3obj.h
  class obj_list_t;
  class obj_t;

  // Forward-declared class available in pub3hilev.h
  class ok_iface_t;

  // Forward-declared class, available in pub3profiler.h
  class profiler_buf_t;

  //-----------------------------------------------------------------------
  
  // Opts can be be a bitmask of the following:
  enum {
    P_DEBUG =    0x1,     /* debug info output w/ text */
    P_IINFO =    0x2,     /* include info output w/ text */
    P_VERBOSE =  0x4,     /* debug messages, etc */
    P_VISERR =   0x8,     /* visible HTML errors */
    P_WSS =      0x10,    /* white-space stripping initializes to **on** */
    P_NOPARSE =  0x20,    /* don't parse file at all */
    P_NOLOCALE = 0x40,    /* Don't localize file */
    P_COPY_CONF= 0x80,    /* copy the config over to universals */
    P_CONFIG =   0x100,   /* run config variables (needed for xml interface) */

    P_OUTPUT_ERR_IN_PLACE = 0x200,          /* output errors in place */
    P_OUTPUT_ERR_PLACEHOLDERS = 0x400,      /* output placeholders in place */
    P_OUTPUT_ERR_COMMENTS = 0x800,          /* output errors in comments */
    P_OUTPUT_ERR_NOLOG= 0x1000,             /* don't warn to stderr */
    P_OUTPUT_ERR_OBJ = 0x2000,              /* pupulate and pub the err-obj */

    P_WARN_INLINE_NULL = 0x4000,            /* warn if %{foo} is NULL */
    P_WARN_NULL = 0x8000,                   /* warn if ever we eval to NULL */
    P_WARN_RELARG_NULL = 0x10000,           /* warn if a relat. arg is NULL */

    P_STRICT_INCLUDE_SCOPING = 0x20000,      /* add scope barrier */
    P_UTF8_JSON = 0x40000,                   /* weird UTF-8 in json */

    P_EXIT_ON_ERROR = 0x80000,               // abort execution as soon as we
                                             // see an error.

    P_INFINITY = 0x100000,

    // All warnings I can think of....
    P_WARN_STRICT = P_WARN_INLINE_NULL | P_WARN_NULL | P_WARN_RELARG_NULL
  };

  //-----------------------------------------------------------------------

  class control_t {
  public:
    control_t () : _break (false), _continue (false), _exit (false) {}
    static ptr<control_t> alloc ();
    bool handle_forloop ();
    void reset_forloop ();
    void reset_file ();
    bool handle_zone ();
    bool _break;
    bool _continue;
    bool _exit;
    void set_rtrn (ptr<const expr_t> x) { _return = x; }
    ptr<const expr_t> rtrn () const { return _return; }
    void set_continue (bool b) { _continue = b;  }
    void set_break (bool b) { _break = b; }
    void set_exit (bool b) { _exit = b; }
    ptr<const expr_t> _return;
  };

  //-----------------------------------------------------------------------

  class lambda_state_t {
  public:
    lambda_state_t () : _binding_stack_size (0), _overflow (false) {}
    bool is_ok () const { return !_overflow; }
    friend class eval_t;
  protected:
    size_t _binding_stack_size;
    ptr<control_t> _old_control;
    bool _overflow;
  };

  //-----------------------------------------------------------------------

  // A runtime location, with the file filled in (metadata), 
  // the currention function call if applicable, and finally,
  // the line number of the file we're currently on;
  class runloc_t {
  public:
    runloc_t (ptr<const metadata_t> md, str fn = NULL) 
      : _metadata (md), _func (fn), _lineno (0), _active (true) {}
    void set_lineno (lineno_t l) { _lineno = l; }
    str filename () const;
    str funcname () const { return _func; }
    lineno_t lineno () const { return _lineno; }
    void pub (obj_t &out) const;
    str to_str () const;
    ptr<const metadata_t> metadata () const { return _metadata; }
    bool set_active (bool b);
    bool active () const { return _active; }
    void profile_report (profiler_buf_t *buf) const;
  private:
    ptr<const metadata_t> _metadata;
    str _func;
    lineno_t _lineno;
    bool _active;
  };

  //-----------------------------------------------------------------------

  class loc_stack_t : public vec<runloc_t> {
  public:
    loc_stack_t ();
    loc_stack_t (const loc_stack_t &l);
    ~loc_stack_t ();
    obj_list_t pub (ssize_t stop = -1) const;
    bool set_active (bool b);
    void profile_report (profiler_buf_t *buf, int64_t sid) const;
    list_entry<loc_stack_t> _lnk;
  };

  //-----------------------------------------------------------------------

  class eval_t : public virtual refcount {
  public:

    enum { EVAL_INIT = -2, EVAL_DONE = -1 };
    ~eval_t ();

    eval_t (ptr<bindtab_t> unis, zbuf *z, opts_t o = 0); // for output
    eval_t (ptr<bindtab_t> unis, ptr<bindtab_t> glbs, 
	    opts_t o = 0);    // for cfg

    eval_t (ptr<env_t> e, ptr<output_t> o, opts_t opts = 0); //everything else

    void publish (str nm, location_t loc,
		  ptr<bind_interface_t> d, status_ev_t ev, CLOSURE);
    void set_pub_iface (ptr<ok_iface_t> i) { _pub_iface = i; }
    bool set_active (bool b);

    void output (zstr s);
    void output (zstr orig, zstr wss);
    void output (str s);
    void output_errs (const xpub3_errstrs_t &e, err_type_t t);
    void output_err_stacktrace (str s, err_type_t t);
    void output_err (str s, err_type_t t);

    ptr<eval_t> clone () const;

    str set_cwd (str s) ;
    str cwd () const { return _cwd; }
    void publish_file (ptr<const file_t> file, status_ev_t ev, CLOSURE);
    ptr<bindtab_t> push_bindings (env_t::layer_type_t lt);
    bool is_config () const;

    ptr<output_t> out () const { return _output; }
    ptr<env_t> env () { return _env; }
    ptr<const env_t> env () const { return _env; }

    bool set_loud (bool b);
    bool loud () const { return _loud && !_silent; }
    bool set_silent (bool b);
    bool silent () const { return _silent; }

    void set_opts (opts_t o) { _opts = o; }
    opts_t opts () const { return _opts; }
    bool utf8_json () const { return _opts & P_UTF8_JSON; }

    ptr<const expr_t> lookup_val (const str &nm) const;
    ptr<mref_t> lookup_ref (const str &nm) const;

    location_t location (lineno_t l) const;
    bool push_muzzle (bool b);
    void pop_muzzle (bool b);

    void report_error (str msg, location_t l);
    void report_error (str msg, lineno_t l);

    // Add the error object from output into the global bindings list
    // (in the environment)
    void add_err_obj (str key);

    // replace the output with a new output engine.
    ptr<output_t> set_output (ptr<output_t> no);

    lambda_state_t 
    push_lambda_call (ptr<const metadata_t>, str fn, 
		      ptr<bindtab_t> bindings, 
		      const env_t::stack_t *cls_stk);

    ptr<const expr_t> pop_lambda_call (lambda_state_t state);

    // manipulate control stack
    ptr<control_t> control ();
    ptr<control_t> push_control ();
    void restore_control (ptr<control_t> c);
    ptr<const metadata_t> current_metadata () const;
    void push_metadata (ptr<const metadata_t> md);
    void pop_metadata ();
    void set_lineno (lineno_t line);
    ptr<ok_iface_t> pub_iface () { return _pub_iface; }

    void clear_me (ptr<expr_t> x) { _to_clear.push_back (x); }
    const loc_stack_t *get_loc_stack () const { return &_stack; }

  protected:
    void clone_env ();
    void silence_output ();

    // A stack of all of the files being published, with their actual
    // metadata.
    location_t _location;        // current location 
    loc_stack_t _stack;

    ptr<env_t> _env;
    ptr<output_t> _output;
    bool _loud;
    bool _silent;
    opts_t _opts;

    ptr<control_t> _control;     // control flow control

    str _cwd;
    ptr<ok_iface_t> _pub_iface;  // publisher interface
    vec<ptr<expr_t> > _to_clear; // to clear on dealloc to clear cycles

  };

  //-----------------------------------------------------------------------
};
