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

#include "aparse.h"

void
async_parser_t::parse (cbi::ptr c)
{
  assert (!parsing);
  pcb = c;
  // this call might set dataready
  abuf->init (wrap (this, &async_parser_t::can_read_cb));
  parsing = true;
  v_debug ();
  if (dataready)
    parse_guts ();
}

void 
async_parser_t::resume ()
{
  parsing = false;
  dataready = false;
  abuf->init (wrap (this, &async_parser_t::can_read_cb));
  parsing = true;
  if (dataready)
    parse_guts ();
}

void
async_parser_t::can_read_cb ()
{
  abuf->can_read ();
  if (parsing)
    parse_guts ();
  else
    dataready = true;
}

void
async_parser_t::cancel ()
{
  if (abuf)
    abuf->cancel ();
  pcb = NULL;
}

void
async_parser_t::finish_parse (int r)
{
  parsing = false;
  if (pcb) {
    cbi::ptr tmp = pcb;
    pcb = NULL;
    (*tmp) (r);
  }
}

void
async_dumper_t::dump (size_t len, cbs c)
{
  buf = New mstr (len);
  bp = buf->cstr ();
  endp = bp + len;

  dump_cb = c;
  parse (wrap (this, &async_dumper_t::parse_done_cb));
}

void
async_dumper_t::parse_guts ()
{
  int rc = 0;
  while (bp < endp) {
    rc = abuf->dump (bp, endp - bp);
    if (rc < 0)
      break; // EOF or WAIT
    bp += rc;
  }
  if (bp == endp || rc == ABUF_EOFCHAR)
    finish_parse (0);
  // otherwise, we're waiting
}

void
async_dumper_t::parse_done_cb (int dummy)
{
  if (dummy == 0)
    buf->setlen (bp - buf->cstr ());

  cbs tcb = dump_cb;
  dump_cb = NULL;

  // calling this callback might cause us to be deleted, so let's not
  // touch any class variables afterwards.
  (*tcb) (*buf);
}
