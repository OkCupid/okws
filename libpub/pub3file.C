
#include "async.h"
#include "crypt.h"
#include "arpc.h"
#include "pub3file.h"
#include "pub3ast.h"
#include "sha1.h"

namespace pub3 {

  //================================== file_t =============================

  ptr<file_t> file_t::alloc (ptr<metadata_t> m, ptr<zone_t> z, opts_t o)
  { return New refcounted<file_t> (m, z, o); }

  //-----------------------------------------------------------------------

  ptr<file_t>
  file_t::raw_alloc (ptr<metadata_t> m, zstr z, opts_t o)
  {
    location_t l (m->jailed_filename (), 1);
    ptr<zone_raw_t> zone = zone_raw_t::alloc (l, z);
    return New refcounted<file_t> (m, zone, o);
  }

  //-----------------------------------------------------------------------

  void
  file_t::init_xdr_opaque ()
  {
    if (!_xdr_opaque) {
      xpub3_file_t file;
      to_xdr (&file);
      _xdr_opaque = xdr2str (file);
      sha1_hash (_xdr_opaque_hash.buf (), 
		 _xdr_opaque.cstr (), _xdr_opaque.len ());
    }
  }

  //-----------------------------------------------------------------------

  ssize_t 
  file_t::xdr_len () const 
  { 
    ssize_t ret = _xdr_opaque ? _xdr_opaque.len () : ssize_t (-1);
    return ret;
  }

  //-----------------------------------------------------------------------

  void file_t::get_xdr_hash (xpub3_hash_t *x) const 
  { _xdr_opaque_hash.to_xdr (x); }

  //-----------------------------------------------------------------------

  ssize_t
  file_t::get_chunk (size_t offset, char *buf, size_t capacity) const
  {
    ssize_t ret;
    if (!_xdr_opaque || offset >= _xdr_opaque.len ()) {
      ret = -1;
    } else {
      ret = min<ssize_t> (_xdr_opaque.len () - offset, capacity);
      memcpy (buf, _xdr_opaque.cstr () + offset, ret);
    }
    return ret;
  }

  //============================= fhash_t =================================

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
      sc.final (h->buf ());
      close (fd);
      ret = true;
    }
    return ret;
  }
#undef BUFSIZE

  //-----------------------------------------------------------------------

  void
  fhash_t::to_xdr (xpub3_hash_t *ph) const
  {
    memcpy (ph->base (), _b, PUBHASHSIZE);
  }

  //-----------------------------------------------------------------------

  str
  fhash_t::to_str_16 () const 
  {
    return strbuf () << hexdump (_b, PUBHASHSIZE); 
  }

  //-----------------------------------------------------------------------

  bool
  fhash_t::operator== (const fhash_t &p2) const
  {
    return (!memcmp ((const void *)_b, (const void *)p2._b, PUBHASHSIZE));
  }
  
  //-----------------------------------------------------------------------

  bool 
  fhash_t::operator== (const xpub3_hash_t &ph) const
  {
    return (!memcmp ((const void *)_b, (void *)ph.base (), PUBHASHSIZE));
  }

  //-----------------------------------------------------------------------

  bool fhash_t::operator!= (const xpub3_hash_t &ph) const
  { return !(*this == ph); }
  
  //-----------------------------------------------------------------------

  bool fhash_t::operator!= (const fhash_t &ph) const
  { return !(*this == ph); }

  //-----------------------------------------------------------------------

  hash_t
  fhash_t::hash_hash () const
  {
    const u_int *p = (const u_int *)_b;
    const char *end_c = _b + PUBHASHSIZE;
    const u_int *end_i = (const u_int *)end_c;
    u_int r = 0;
    while (p < end_i)
      r = r ^ *p++;
    return r;
  }
  
  //----------------------------------------------------------------------

  ptr<fhash_t>
  file2hash (const str &fn, struct stat *sbp)
  {
    ptr<fhash_t> p = New refcounted<fhash_t> ();
    if (!file2hash (fn, p, sbp)) p = NULL;
    return p;
  }
  
  //=============================================== metadata_t ===========

  ptr<expr_dict_t>
  metadata_t::to_dict () const
  {
    pub3::obj_dict_t i;
    if (_ifn) { i ("input_filename") = _ifn; }
    if (_jfn) { i ("jailed_filename")  = _jfn; }
    if (_rfn) { i ("real_filename") = _rfn; }
    if (_hsh) { i ("hash") = _hsh->to_str_16 (); }
    i ("ctime") = int64_t (_ctime);
    return i.to_dict ();
  }

  //---------------------------------------------------------------------

  ptr<expr_dict_t>
  metadata_t::to_binding () const
  {
    pub3::obj_dict_t d;
    d ("metadata") = obj_dict_t (to_dict ());
    return d.to_dict ();
  }

  //---------------------------------------------------------------------

  str metadata_t::jailed_filename () const { return _jfn; }
  str metadata_t::real_filename () const { return _rfn; }

  //================================================= jailer_t ==========

  bool jailer_t::_in_jail;

  //---------------------------------------------------------------------

  ptr<jailer_t> jailer_t::alloc () { return New refcounted<jailer_t> (); }

  //---------------------------------------------------------------------

  jailer_t::jailer_t (jail_mode_t m, str d) : _mode (m), _dir (d) {}

  //---------------------------------------------------------------------

  void jailer_t::set_in_jail (bool b) { _in_jail = b; }

  //---------------------------------------------------------------------

  void jailer_t::setjail (jail_mode_t m, str d)
  { _mode = m; _dir = d; }

  //---------------------------------------------------------------------

  bool jailer_t::be_verbose () { return OKDBG2(OKD_JAIL); }

  //---------------------------------------------------------------------

  str
  jailer_t::jail2real (str in) const
  {
    str ret = in;
    if (in && _dir && (!_in_jail || _mode == JAIL_VIRTUAL)) { 
      ret = dir_merge (_dir, in); 
    }
    if (in && ret) {
      OKDBG4(OKD_JAIL, CHATTER,  "okws-jail: %s -> %s\n", 
	     in.cstr (), ret.cstr ());
    }
    return ret;
  }

  //=====================================================================


};
