

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

class http_hdr_t : public pairtab_t<> {
public:
  http_hdr_t (abuf_t *a, size_t bfln = HTTPHDR_DEF_SCRATCH, char *b = NULL)
    : abuf (a), 
      buflen (min<size_t> (bfln, HTTPHDR_MAX_SCRATCH)), 
      scratch (b), scratchalloc (false), hdrend (false),
      parsing (false), dataready (false), noins (false), nvers (0)
  {
    if (!scratch) {
      scratch = (char *) xmalloc (buflen);
      scratchalloc = true;
    }
    pcp = scratch;
    endp = scratch + buflen;
  }

  ~http_hdr_t () 
  { 
    if (scratchalloc && scratch) xfree (scratch); 
  }

  virtual void parse (cbi::ptr c);
  virtual void can_read_cb ();
  virtual void cancel ();

  inline htpv_t get_vers () const { return nvers; }

  int contlen;     // content-length size

 protected:
  abuf_stat_t delimit_word (str *d, bool qms = false);
  abuf_stat_t delimit_key (str *k);
  abuf_stat_t delimit_val (str *v);
  bool eol () ;
  bool gobble_eol ();

  inline bool iscookie () const
  {
    return (key && key.len () == 6 && mystrlcmp (key, "cookie"));
  }

  virtual void _parse () = 0;
  virtual void fixup ();

  abuf_t *abuf;
  size_t buflen;
  char *scratch;
  bool scratchalloc;

  char *pcp;        // parse character pointer
  char *endp;       // end of the scratch buffer
  str key, val;     // used in parsing key/val pairs
  str vers;         // HTTP version
  bool hdrend;      // hit end-of-header

  cbi::ptr pcb;     // call this CB after parse is done
  bool parsing;     // flag is on if parsing
  bool dataready;   // on if we have data ready to read
  
  bool noins;       // on to disable insert into the pairtab (for Cookies)

  char scr2[SCR2_LEN]; // scratch for logging purposes, etc
  htpv_t nvers;     // HTTP version; 0 ==> 1.0,  1 ==> 1.1
};


#endif /* _LIBAHTTP_HDR_H */
