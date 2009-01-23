// -*-c++-*-
/* $Id: hdr.h 2148 2006-08-11 20:09:48Z max $ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#ifndef __LIBAHTTP_OKSCRATCH_H__
#define __LIBAHTTP_OKSCRATCH_H__

#include "async.h"
#include "list.h"
#include "ihash.h"

namespace ok {

  //-----------------------------------------------------------------

  class scratch_t  {
  public:
    scratch_t (size_t l) : _buf (New char[l]), _len (l) {}
    ~scratch_t () { delete [] _buf; }
    char *buf () { return _buf; }
    char *end () { return _buf + _len ; }
    const char *end () const { return _buf + _len ; }
    const char *buf () const { return _buf; }
    size_t len () const { return _len; }
    list_entry<scratch_t> _lnk;
  private:
    char *_buf;
    size_t _len;
  };

  //-----------------------------------------------------------------

  class scrlist_t {
  public:
    scrlist_t (size_t sz) : _size (sz) {}
    ~scrlist_t ();

    scratch_t *alloc ();
    void reclaim (scratch_t *s);

    size_t _size;
    ihash_entry<scrlist_t> _hlnk;
    list<scratch_t, &scratch_t::_lnk> _lst;
  };

  //-----------------------------------------------------------------------

  class scratch_handle_t;

  class scratch_mgr_t {
  public:
    scratch_mgr_t () {}
    ~scratch_mgr_t () { _h.deleteall (); }

    ptr<scratch_handle_t> alloc (size_t s);
    static ptr<scratch_handle_t> nonstd_alloc (size_t s);
    friend class scratch_handle_t;
  private:
    void reclaim (scratch_t *s);

    ihash<size_t, scrlist_t, &scrlist_t::_size, &scrlist_t::_hlnk> _h;
  };

  //-----------------------------------------------------------------

  class scratch_handle_t : public virtual refcount {
  public:
    scratch_handle_t (scratch_t *s, scratch_mgr_t *m)
      : _scratch (s),
	_mgr (m) {}

    ~scratch_handle_t ();

    char *buf () { return _scratch->buf (); }
    char *end () { return _scratch->end (); }
    const char *buf () const { return _scratch->buf (); }
    const char *end () const { return _scratch->end (); }
    size_t len () const { return _scratch->len (); }
    ptr<scratch_handle_t> grow (size_t sz);

  private:
    scratch_t *_scratch;
    scratch_mgr_t *_mgr;
  };

  //-----------------------------------------------------------------

  ptr<scratch_handle_t> alloc_scratch (size_t sz);
  ptr<scratch_handle_t> alloc_nonstd_scratch (size_t sz);

  //-----------------------------------------------------------------

};


#endif /* __LIBAHTTP_OKSCRATCH_H__ */
