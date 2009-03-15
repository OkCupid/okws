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

#ifndef _LIBAHTTP_HDR_H
#define _LIBAHTTP_HDR_H

#define HTTPHDR_DEF_SCRATCH 0x10400
#define HTTPHDR_MAX_SCRATCH 0x40000

#define SCR2_LEN 1024

#include "abuf.h"
#include "pair.h"
#include "ahutil.h"
#include "qhash.h"
#include "aparse.h"

//
// somewhat mislabeled -- an HTTP header parser; note no storage
// involved
//
class http_hdr_t : public virtual async_parser_t {
public:
  http_hdr_t (abuf_t *a, ptr<ok::scratch_handle_t> s)
    : async_parser_t (a),
      _scratch (s ? s : ok::alloc_scratch (HTTPHDR_DEF_SCRATCH)),
      pcp (_scratch->buf ()),
      endp (_scratch->end ()),
      noins (false), 
      _scr2 (ok::alloc_scratch (SCR2_LEN)),
      nvers (0),
      CRLF_need_LF (false)
      
  {}

  void reset ();

  ~http_hdr_t () {}

  inline htpv_t get_vers () const { return nvers; }
  
 protected:
  abuf_stat_t delimit_word (str *d, bool qms = false);
  abuf_stat_t delimit_key (str *k);
  abuf_stat_t delimit_val (str *v);
  abuf_stat_t delimit_status (str *v);
  abuf_stat_t delimit (str *k, char stopchar, bool tol, bool gobble);
  abuf_stat_t eol () ;
  abuf_stat_t gobble_crlf (); 
  abuf_stat_t require_crlf ();
  abuf_stat_t force_match (const char *s, bool tol = true);

  inline bool iscookie () const
  {
    return (key && key.len () == 6 && mystrlcmp (key, "cookie"));
  }


  ptr<ok::scratch_handle_t> _scratch;

  char *pcp;        // parse character pointer
  char *endp;       // end of the scratch buffer
  str key, val;     // used in parsing key/val pairs
  str vers;         // HTTP version

  cbi::ptr pcb;     // call this CB after parse is done
  
  bool noins;       // on to disable insert into the pairtab (for Cookies)

  ptr<ok::scratch_handle_t> _scr2;
  htpv_t nvers;     // HTTP version; 0 ==> 1.0,  1 ==> 1.1

  bool CRLF_need_LF; // when looking for a CRLF....

  const char *curr_match; // state for force_match function
  const char *fmcp;
};


#endif /* _LIBAHTTP_HDR_H */
