// -*-c++-*-
/* $Id$ */

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

#ifndef _LIBAHTTP_APARSE_H
#define _LIBAHTTP_APARSE_H

#include "abuf.h"

//
// Asynchronous Parser Skeleton Class
//
class async_parser_t {
public:
  async_parser_t () 
    : abuf (NULL), nwabuf (false), parsing (false), dataready (false) {}
  async_parser_t (abuf_src_t *a)
    : abuf (New abuf_t (a)), nwabuf (true), parsing (false), 
      dataready (false) {}
  async_parser_t (abuf_t *a) 
    : abuf (a), nwabuf (false), parsing (false), dataready (false) {}
  virtual ~async_parser_t () { if (nwabuf) delete (abuf); }
  
  void parse (cbi::ptr c = NULL);
  void can_read_cb ();
  void cancel ();
  void finish_parse (int r = 0);
  void resume ();

  void reset () 
  { 
    if (abuf) abuf->reset ();
    parsing = dataready = false; 
  }
  
  // where the real guts of the async parsing is done -- not here
  virtual void parse_guts () = 0;
  abuf_t *get_abuf () { return abuf; }

protected:
  abuf_t *abuf;
private:
  cbi::ptr pcb;
  bool nwabuf;
  bool parsing;
  bool dataready;
};

class async_dumper_t : public async_parser_t {
public:
  async_dumper_t (abuf_t *a) : async_parser_t (a), buf (NULL) {}
  ~async_dumper_t () { if (buf) delete buf; }

  void dump (size_t len, cbs c);
protected:
  void parse_guts ();
  void parse_done_cb (int dummy) { (*dump_cb) (*buf); dump_cb = NULL; }

  mstr *buf;
  char *bp, *endp;
  cbs::ptr dump_cb;
};

#endif
