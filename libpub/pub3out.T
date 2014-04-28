// -*-c++-*-

#include "pub3out.h"
#include "pescape.h"


namespace pub3 {

  //========================================== output_t ==================

  output_t::output_t (opts_t o) :
    _opts (o),
    _muzzle (false),
    _wss_enabled (o & P_WSS) {}

  //--------------------------------------------------------------------

  void
  output_t::set_opts (opts_t o)
  {
    _opts = o;
    _wss_enabled = (o & P_WSS);
  }

  //--------------------------------------------------------------------

  opts_t output_t::get_opts () const { return _opts; }

  //--------------------------------------------------------------------

  bool output_t::get_wss_enabled () const { return _wss_enabled; }

  //--------------------------------------------------------------------

  ptr<output_t>
  output_t::alloc (zbuf *z, opts_t o)
  {
    ptr<output_t> ret;
    if (z) { ret = New refcounted<output_std_t> (z, o);   }
    else   { ret = New refcounted<output_silent_t> (o); }
    return ret;
  }

  //--------------------------------------------------------------------

  bool
  output_t::push_muzzle (bool b)
  {
    bool ret = _muzzle;
    if (b && !_muzzle) { _muzzle = b; }
    return ret;
  }

  //--------------------------------------------------------------------

  void output_t::pop_muzzle (bool b) { _muzzle = b; }

  //--------------------------------------------------------------------

  static str
  typ2str (err_type_t typ)
  {
    const char *ret = "other";
    switch (typ) {
    case P_ERR_WARNING: ret = "warning"; break;
    case P_ERR_PARSE: ret = "parse"; break;
    case P_ERR_ERROR: ret = "error"; break;
    case P_ERR_EVAL: ret = "eval"; break;
    default: break;
    }
    return str (ret);
  }

  //--------------------------------------------------------------------

  void
  output_t::pub3_add_error (const loc_stack_t &stk, str msg, err_type_t typ)
  {
    if (stk.size ()) {
      pub3::obj_dict_t o;
      stk.back ().pub (o);
      o("error") = msg;
      o("type") = typ2str (typ);
      pub3::obj_list_t ps = stk.pub (-1);
      o("stack") = ps;
      _err_obj.push_back (o);
    }
  }

  //--------------------------------------------------------------------

  static str 
  html_err (str s)
  {
    strbuf b;
    b << "<font color=\"red\">[" << xss_escape(s) << "]</font>";
    return b;
  }

  //--------------------------------------------------------------------

  static str
  html_comment (str s)
  {
    strbuf b;
    b << "<!-- " << s << "-->";
    return b;
  }

  //--------------------------------------------------------------------

  void
  output_t::output_visible_error (str s)
  {
    if (_opts & P_OUTPUT_ERR_PLACEHOLDERS) {
      strbuf b; 
      b << "[" << _err_obj.size () << "]";
      output (html_err (b));
    }

    if (_opts & P_OUTPUT_ERR_IN_PLACE) {
      output (html_err (s));
    }

    if (_opts & P_OUTPUT_ERR_COMMENTS) {
      output (html_comment (s)); 
    }
  }

  //--------------------------------------------------------------------

  void
  output_t::output_err (runloc_t loc, str msg, err_type_t t)
  {
    loc_stack_t tmp_stack;
    tmp_stack.push_back (loc);
    output_err (tmp_stack, msg, t);
  }

  //--------------------------------------------------------------------

  void
  output_t::output_err (str msg, err_type_t t)
  {
    loc_stack_t tmp_stack;
    output_err (tmp_stack, msg, t);
  }

  //--------------------------------------------------------------------

  void
  output_t::output_err (const loc_stack_t &stk, str msg, err_type_t t)
  {
    if (_opts & P_OUTPUT_ERR_OBJ) {
      pub3_add_error (stk, msg, t);
    }

    str typ = typ2str (t);

    str loc;
    if (stk.size ()) {
      strbuf b;
      runloc_t top = stk.back ();
      str tl = top.to_str ();
      b <<  tl << ": ";
      loc = b;
    } else {
      loc = "";
    }

    strbuf tmp;
    tmp << "okws-pub3[" << typ << "]: ";
    str prfx = tmp;

    strbuf logline;

    logline << prfx << loc << msg;

    output_visible_error (logline);

    if (!(_opts & P_OUTPUT_ERR_NOLOG)) {
      warn << logline << "\n";

      ssize_t i = stk.size () - 2;
      ssize_t lim = max<ssize_t> (0, i - 10);

      for ( ; i >= lim; i--) {
	loc = stk[i].to_str ();
	warn << prfx << "    #" << i << ": " << loc << "\n";
      }
    }
  }

  //-----------------------------------------------------------------------

  ptr<expr_t> output_t::err_obj () { return _err_obj.obj (); }

  //-----------------------------------------------------------------------

  bool
  output_t::enable_wss (bool b)
  {
    bool ret = _wss_enabled;
    _wss_enabled = b;
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  output_t::wss_boundary (bool on, str tag)
  {
    if (on && !_wss_boundary) {
      _wss_boundary = tag;
    } else if (!on && _wss_boundary && cicmp (_wss_boundary, tag)) {
      _wss_boundary = NULL;
    }
    return true;
  }

  //-----------------------------------------------------------------------

  void
  output_t::output (zstr orig, zstr wss) 
  {
    output (do_wss () ? wss : orig);
  }

  //-----------------------------------------------------------------------

  bool
  output_t::do_wss () const
  {
    return _wss_enabled && !_wss_boundary;
  }

  //=================================== output_std_t ====================

  void output_std_t::output (zstr z) { if (z && !_muzzle) _out->cat (z); }
  void output_std_t::output (str s) { if (s && !_muzzle) _out->cat (s); }

  //=================================== output_silent_t ================

  ptr<output_silent_t> output_silent_t::alloc (opts_t o) 
  { return New refcounted<output_silent_t> (o); }

  //====================================================================

};
