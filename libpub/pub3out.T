// -*-c++-*-

#include "pub3out.h"


namespace pub3 {

  //========================================== output_t ==================

  ptr<output_t>
  output_t::alloc (zbuf *z)
  {
    ptr<output_t> ret;
    if (z) { ret = New refcounted<output_std_t> (z);   }
    else   { ret = New refcounted<output_silent_t> (); }
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
    b << "<font color=\"red\">[" << s << "]</font>";
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
    pub3_add_error (stk, msg, t);

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

  //=================================== output_std_t ====================

  void output_std_t::output (zstr z) { if (z) _out->cat (z); }
  void output_std_t::output (str s) { if (s) _out->cat (s); }

  //=================================== output_silent_t ================

  ptr<output_silent_t> output_silent_t::alloc () 
  { return New refcounted<output_silent_t> (); }

  //====================================================================

};
