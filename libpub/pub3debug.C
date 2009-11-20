#include "pub3debug.h"

namespace pub3 {

  //----------------------------------------------------------------------
  
  void
  dumper_t::begin_obj (str s, void *p, lineno_t l)
  {
    _buf.fmt ("%s-{ %s (%p", _space.cstr (), s.cstr (), p);
    _hold.push_back (s);
    if (l) _buf << "," << l;
    _buf << ")\n";
    inc_level ();
  }
  
  //----------------------------------------------------------------------

  void
  dumper_t::inc_level () 
  {
    _level ++ ;
    setspace ();
  }

  //----------------------------------------------------------------------

  void
  dumper_t::setspace ()
  {
    str *sp = _spaces[++_level];
    if (!sp) {
      size_t ns = _level * INDENT;
      mstr m (ns);
      memset (m.cstr (), ' ', ns);
      _space = m;
      _spaces.insert (_level, _space);
    } else {
      _space = *sp;
    }
  }

  //----------------------------------------------------------------------

  dumper_t::dumper_t () : _level (0) { setspace (); }

  //----------------------------------------------------------------------

  void 
  dumper_t::dec_level ()
  {
    _space = *_spaces[--_level];
  }

  //----------------------------------------------------------------------

  void
  dumper_t::end_obj ()
  {
    _buf.fmt ("%s}\n", _space.cstr ());
  }
  
  //----------------------------------------------------------------------

  void
  dumper_t::dump (str s, bool nl)
  {
    _buf << _space << s;
    _hold.push_back (s);
    if (nl) _buf << "\n";
  }
  
  //----------------------------------------------------------------------

};
