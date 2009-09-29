#include "pub3file.h"
#include "sha1.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  ptr<file_t> file_t::alloc (ptr<metadata_t> m, ptr<zone_t> z)
  { return New refcounted<file_t> (m, z); }

  //-----------------------------------------------------------------------

#define BUFSIZE 4096

  bool
  file2hash (const str &fn, fhash_t *h, struct stat *sbp)
  {
    char buf[BUFSIZE];
    struct stat sb;
    bool ret = false;
    int fd;

    if (!sbp)
      sbp = &sb;
    if (access (fn.cstr (), F_OK | R_OK) != 0) {
      /* noop */
    } else if (stat (fn.cstr (), sbp) != 0 || !S_ISREG (sbp->st_mode)) {
      /* noop */
    } else if ((fd = open (fn.cstr (), O_RDONLY)) < 0) {
      /* noop */
    } else {
      size_t n;
      sha1ctx sc;
      while ((n = read (fd, buf, BUFSIZE))) {
	sc.update (buf, n);
      }
      sc.final (h->buffer ());
      close (fd);
      ret = true;
    }
    return ret;
  }
#undef BUFSIZE

  //-----------------------------------------------------------------------

  bool
  fhash_t::operator== (const fhash_t &p2) const
  {
    return (!memcmp ((void *)val, (void *)p2.val, PUBHASHSIZE));
  }
  
  //-----------------------------------------------------------------------

  bool 
  fhash_t::operator== (const xpub3_hash_t &ph) const
  {
    return (!memcmp ((void *)val, (void *)ph.base (), PUBHASHSIZE));
  }

  //-----------------------------------------------------------------------

  bool fhash_t::operator!= (const xpub3_hash_t &ph) const
  {
    return !(*this == ph);
  }
  
  //-----------------------------------------------------------------------

  bool fhash_t::operator!= (const xpub3_hash_t &p2) const
  {
    return !(*this == p2);
  }

  //-----------------------------------------------------------------------

  hash_t
  fhash_t::hash_hash () const
  {
    u_int *p = (u_int *)val;
    const char *end_c = val + PUBHASHSIZE;
    u_int *end_i = (u_int *)end_c;
    u_int r = 0;
    while (p < end_i)
      r = r ^ *p++;
    return r;
  }
  
  //-----------------------------------------------------------------------

  ptr<phash_t>
  file2hash (const str &fn, struct stat *sbp)
  {
    ptr<phash_t> p = New refcounted<phash_t> ();
    if (!file2hash (fn, p, sbp)) p = NULL;
    return p;
  }
  
  //-----------------------------------------------------------------------


};
