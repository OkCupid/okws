
// -*-c++-*-
/* $Id$ */

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
  http_hdr_t (abuf_t *a, size_t bfln = HTTPHDR_DEF_SCRATCH, char *b = NULL)
    : async_parser_t (a),
      buflen (min<size_t> (bfln, HTTPHDR_MAX_SCRATCH)), 
      scratch (b), scratchalloc (false), 
      noins (false), nvers (0),
      CRLF_need_LF (false)
  {
    if (!scratch) {
      scratch = (char *) xmalloc (buflen);
      scratchalloc = true;
    }
    pcp = scratch;
    endp = scratch + buflen;
  }

  void reset ();

  ~http_hdr_t () 
  { 
    if (scratchalloc && scratch) xfree (scratch); 
  }

  inline htpv_t get_vers () const { return nvers; }
  
 protected:
  abuf_stat_t delimit_word (str *d, bool qms = false);
  abuf_stat_t delimit_key (str *k);
  abuf_stat_t delimit_val (str *v);
  abuf_stat_t delimit (str *k, char stopchar, bool tol, bool gobble);
  abuf_stat_t eol () ;
  abuf_stat_t gobble_crlf (); 
  abuf_stat_t require_crlf ();
  abuf_stat_t force_match (const char *s, bool tol = true);

  inline bool iscookie () const
  {
    return (key && key.len () == 6 && mystrlcmp (key, "cookie"));
  }


  size_t buflen;
  char *scratch;
  bool scratchalloc;

  char *pcp;        // parse character pointer
  char *endp;       // end of the scratch buffer
  str key, val;     // used in parsing key/val pairs
  str vers;         // HTTP version

  cbi::ptr pcb;     // call this CB after parse is done
  
  bool noins;       // on to disable insert into the pairtab (for Cookies)

  char scr2[SCR2_LEN]; // scratch for logging purposes, etc
  htpv_t nvers;     // HTTP version; 0 ==> 1.0,  1 ==> 1.1

  bool CRLF_need_LF; // when looking for a CRLF....

  const char *curr_match; // state for force_match function
  const char *fmcp;
};


#endif /* _LIBAHTTP_HDR_H */
