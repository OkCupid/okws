
#include "abuf.h"

void
abuf_t::moredata ()
{
  if (len > 0)
    src->rembytes (len);
  abuf_indata_t in = src->getdata ();
  buf = cp = in.bp;
  len = in.len;
  endp = buf + in.len;
  erc = in.erc;
}

void 
abuf_t::finish ()
{
  if (!ignfn) {
    src->rembytes (cp - buf);
    src->finish ();
  }
}

void
abuf_con_t::init (cbv c)
{
  cb = c;
  x->setrcb (wrap (this, &abuf_con_t::readcb));
}

void
abuf_con_t::readcb (int n)
{
  if (n == 0) 
    eof = true;
  if (cb)
    (*cb) ();
}

abuf_indata_t
abuf_con_t::getdata ()
{
  if (eof) return abuf_indata_t ();
  return abuf_indata_t (in);
}

abuf_indata_t::abuf_indata_t (suiolite *in)
{
  bp = in->getdata (&len);
  erc = len ? ABUF_OK : ABUF_WAIT;
}

abuf_indata_t
abuf_str_t::getdata ()
{
  if (eof) 
    return abuf_indata_t ();
  else {
    eof = true;
    return abuf_indata_t (ABUF_OK, s, len);
  }
}

abuf_str_t::abuf_str_t (const char *ss, bool cp, int l) : len (l)
{
  if (!len && ss)
    len = strlen (ss);
  if (cp) {
    alloc = true;
    char *t = New char[len];
    memcpy (t, ss, len);
    s = t;
  } else {
    s = ss;
  }
  eof = len > 0;
}

void
abuf_t::mirror (char *p, u_int len)
{
  mirror_base = mirror_p = p;
  mirror_end = mirror_base + len;
}

str
abuf_t::end_mirror ()
{
  while ((mirror_p > mirror_base) && 
	 (mirror_p[-1] == '\r' || mirror_p[-1] == '\n'))
    mirror_p--;
  str r = str (mirror_base, mirror_p - mirror_base);
  mirror_base = mirror_p = mirror_end = NULL;
  return r;
}
