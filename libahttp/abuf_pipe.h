
// -*-c++-*-
/* $Id: abuf.h 1841 2006-05-26 04:18:00Z max $ */

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

/*
 * Warning: including this file will cause stdin,stdout and stderr to
 * become asynchronous (since including 'aios.h' has that effect.
 */

#ifndef _LIBAHTTP_ABUF_PIPE_H
#define _LIBAHTTP_ABUF_PIPE_H

#include "abuf.h"
#include "aios.h"

class abuf_pipe_t : public abuf_src_t {
public:
  abuf_pipe_t (ref<aios> s) : _aios (s), _eof (false), 
			      _destroyed (New refcounted<bool> (false)),
			      _read_outstanding (false),
			      _buf_pos (0), _next_buf (0) {}
  void init (cbv cb);
  abuf_indata_t getdata ();
  void rembytes (int nbytes);
  void finish ();
  void cancel () { _cb = NULL; }
  ~abuf_pipe_t () { *_destroyed = true; finish (); }
  bool overflow () const { return false; }
private:
  void schedule_read ();
  void readcb (str s, int err);
  ref<aios> _aios;
  cbv::ptr _cb;
  vec<str> _bufs;
  bool _eof;
  ptr<bool> _destroyed;
  bool _read_outstanding;

  size_t _buf_pos, _next_buf;
};

#endif /* _LIBAHTTP_ABUFPIPE_H */
