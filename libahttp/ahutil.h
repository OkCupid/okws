
// -*-c++-*-
/* $Id$ */

#ifndef _LIBAHTTP_AHUTIL
#define _LIBAHTTP_AHUTIL 1

#include "async.h"

typedef u_char htpv_t;

str getdate ();
bool mystrlcmp (const str &s, const char *b);
str tolower (const str &in);

inline 
int char_at (const str &s, int i)
{ 
  if (!s) return -1;
  i = (i < 0) ? s.len () + i : i;
  return (i < 0 || i >= int (s.len ())) ? -1 : s[i];
}

void stall (int sig, cbv c);
void stall (const str &fn, cbv c);

class cbbun_t : public virtual refcount {
public:
  cbbun_t (u_int i, cbv c) : cnt (i), cb (c), called (false) 
  { 
    if (!i) {
      called = true;
      (*c) (); 
    }
  }
   
  void dec (ptr<cbbun_t> d = NULL)
  {
    if (!called && !--cnt) {
      called = true;
      (*cb) ();
    }
  }

  cbv::ptr bwrap ()
  {
    return wrap (this, &cbbun_t::dec, mkref (this)); 
  }

  static ptr<cbbun_t> alloc (u_int i, cbv c) 
  { return New refcounted<cbbun_t> (i, c); }

private:
  u_int cnt;
  cbv cb;
  bool called;
};

#endif /* _LIBAHTTP_AHUTIL */
