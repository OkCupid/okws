#include "pub3debug.h"

namespace pub3 {

  //----------------------------------------------------------------------
  
  void
  dumper_t::begin_obj (str s, const void *p, lineno_t l)
  {
    strbuf b;
    b.fmt ("-{ %s (%p", s.cstr (), p);
    if (l) b << "," << l;
    b << ")";
    dump (b, true);
    inc_level ();
  }
  
  //----------------------------------------------------------------------

  void
  dumper_t::inc_level () 
  {
    _level ++ ;
  }

  //----------------------------------------------------------------------

  str
  dumper_t::get_space (size_t l)
  {
    str ret;
    str *sp = _spaces[l];
    if (!sp) {
      size_t ns = l * INDENT;
      mstr m (ns);
      memset (m.cstr (), ' ', ns);
      m.setlen (ns);
      ret = m;
      _spaces.insert (l, ret);
    } else {
      ret = *sp;
    }
    return ret;
  }

  //----------------------------------------------------------------------

  dumper_t::dumper_t () : _level (0) {}

  //----------------------------------------------------------------------

  void 
  dumper_t::dec_level ()
  {
    --_level;
  }

  //----------------------------------------------------------------------

  void
  dumper_t::end_obj ()
  {
    dec_level ();
    dump ("}", true);
  }
  
  //----------------------------------------------------------------------

  ptr<dumper_t::line_t>
  dumper_t::get_curr () 
  {
    if (!_curr) { _curr = New refcounted<line_t> (_level); }
    return _curr;
  }

  //----------------------------------------------------------------------

  void
  dumper_t::push_curr ()
  {
    if (_curr) { _lines.push_back (_curr); }
    _curr = NULL;
  }

  //----------------------------------------------------------------------

  void
  dumper_t::line_t::append (str s)
  {
    if (s) {
      _v.push_back (s);
      _b << s;
    }
  }

  //----------------------------------------------------------------------

  void
  dumper_t::dump (str s, bool nl)
  {
    get_curr ()->append (s);
    if (nl) { push_curr (); }
  }
  
  //----------------------------------------------------------------------

  void
  dumper_t::dump_to (strbuf &b)
  {
    push_curr ();
    for (size_t i = 0; i < _lines.size (); i++) {
      ptr<line_t> l = _lines[i];
      str space = get_space (l->level ());
      str content = l->to_str ();
      b << space << content << "\n";
    }
  }

  //----------------------------------------------------------------------

  void
  dumper_t::dump_to_stderr (const dumpable_t *e)
  {
    dumper_t d;
    e->dump (&d);
    warnobj w ( (int) ::warnobj::xflag);
    d.dump_to (w);
  }

  //=======================================================================

  void
  dumpable_t::s_dump (dumper_t *d, str prfx, const dumpable_t *obj)
  {
    if (prfx) { d->dump (strbuf ("%s ", prfx.cstr ()), false); }
    if (obj) { obj->dump (d); }
    else { d->dump ("(null)", true); }
  }

  //----------------------------------------------------------------------

  void
  dumpable_t::dump (dumper_t *d) const
  {
    d->begin_obj (get_obj_name (), static_cast<const void *> (this), 
		  dump_get_lineno());
    v_dump (d);
    d->end_obj ();
  }

  //=======================================================================

};
