
/* $Id$ */

#include "ok.h"

const char *typ_arr[6] = { "ok", "warning", "error", "notice", 
			   "critical", "debug" };
u_int typ_larr[6] = { 2, 7, 5, 6, 8, 5 };

logbuf_t &
log (logbuf_t *b, oklog_typ_t typ)
{
  return b->bcpy (typ_arr[int (typ)], typ_larr[int (typ)]);
}

bool
mstr2::resize (size_t ul)
{
  size_t sz = len ();
  size_t sz2 = (sz << 1) + 1;
  warn ("XXX: resize %u, %u, %u\n", ul, sz, sz2); // DEBUG
  if (sz2 > LOG_BUF_MAXLEN)
    return false;
  b2 = strobj::alloc (sz + 1);
  memcpy (b2->dat (), b->dat (), ul);
  b = b2;
  alloc_sz = sz;
  return true;
}

bool
logbuf_t::resize ()
{
  u_int dlen = cp - buf;
  if (!cmbuf->resize (dlen))
    return false;
  buf = *cmbuf;
  ep = buf + cmbuf->len ();
  cp = buf + dlen;
  return true;
}

bool
logbuf_t::output (int fd)
{
  uio.clear ();
  uio.print (buf, cp - buf);
  int rc;
  while ((rc = uio.output (fd)) == -1 && errno == EAGAIN) ;
  clear ();
  return (rc >= 0);
}

void
logbuf_t::clearbuf ()
{
  buf = cp = ep = NULL;
  cmbuf = NULL;
  cmbuf_i = -1;
}

bool
logbuf_t::getbuf ()
{
  if ((cmbuf = bufs.get (&cmbuf_i))) {
    buf = cp = *cmbuf;
    ep = cp + cmbuf->len ();
    return true;
  } else {
    clearbuf ();
    return false;
  }
}

mstr2 *
mstrs::get (int *i)
{
  if (frlst.size ()) {
    *i = frlst.pop_back ();
    return bufs[*i];
  } else
    return NULL;
}

int
logbuf_t::lock ()
{
  u_int i = cmbuf_i;
  if (!getbuf ()) 
    warn << "** Ran out of buffers! Logger is bogged down\n";
  return i;
}

bool
logbuf_t::to_str (str *s, int *i)
{
  if (!cmbuf) {
    *i = -1;
    return false;
  } else {
    cmbuf->setlen (cp - buf);
    *s = *cmbuf;                // sets cmbuf->b = NULL
    cmbuf->restore ();          // brings us back from the dead!
    *i = lock ();
  }
  return true;
}


