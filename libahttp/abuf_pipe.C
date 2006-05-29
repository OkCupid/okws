/* $Id: abuf.C 1841 2006-05-26 04:18:00Z max $ */

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


#include "abuf_pipe.h"

void
abuf_pipe_t::init (cbv c)
{
  _cb = c;
  schedule_read ();
}

void
abuf_pipe_t::schedule_read ()
{
  if (!_eof) {
    _read_outstanding = true;
    _aios->readany (wrap (this, &abuf_pipe_t::readcb));
  }
}

void
abuf_pipe_t::finish ()
{
  if (_read_outstanding) {
    _aios->readcancel ();
  }
}

void
abuf_pipe_t::readcb (str s, int err)
{
  _read_outstanding = false;
  if (!s) {
    _eof = true;
  } else {
    _bufs.push_back (s);
  }

  ptr<bool> df = _destroyed;
  if (_cb) (*_cb) ();
  if (!*df) schedule_read ();
}

abuf_indata_t 
abuf_pipe_t::getdata ()
{
  if (_eof) return abuf_indata_t ();
  else {
    if (_next_buf < _bufs.size ()) {
      abuf_indata_t r (ABUF_OK, _bufs[_next_buf].cstr (), 
		       _bufs[_next_buf].len ());
      _next_buf ++;
      return r;
    } else {
      return abuf_indata_t (ABUF_WAIT);
    }
  }
}

void
abuf_pipe_t::rembytes (int nbytes)
{
  while (_bufs.size () && _buf_pos + nbytes >= _bufs[0].len ()) {
    assert (_bufs[0].len () > _buf_pos);
    nbytes -= (_bufs[0].len () - _buf_pos);
    _bufs.pop_front ();
    _next_buf --;
    _buf_pos = 0;
  }
  assert (nbytes <= int (_buf_pos));
  assert (_bufs.size () == 0 || _bufs[0].len () > _buf_pos);
  if (nbytes)
    _buf_pos -= nbytes;
}

