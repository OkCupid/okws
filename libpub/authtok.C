
#include "authtok.h"
#include "sha1.h"

ptr<okauthtok_t>
authtok_alloc (const str &s)
{
  ptr<okauthtok_t> tok = New refcounted<okauthtok_t> ();
  sha1_hash (tok->base (), s.cstr (), s.len ());
  return tok;
}

bool
authtok_cmp (const okauthtok_t &t1, const okauthtok_t &t2)
{
  return (memcmp (t1.base (), t2.base (), OKAUTHTOKSIZE) == 0);
}
