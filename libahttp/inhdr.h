

// -*-c++-*-
/* $Id$ */

#ifndef _LIBAHTTP_INHDR_H
#define _LIBAHTTP_INHDR_H

#define HTTPHDR_DEF_SCRATCH 0x10400
#define HTTPHDR_MAX_SCRATCH 0x40000

#define SCR2_LEN 1024

#include "abuf.h"
#include "pair.h"
#include "cgi.h"
#include "ahutil.h"
#include "qhash.h"

typedef enum { HDRST_START = 0,
	       HDRST_SPC1 = 1,
	       HDRST_TARGET = 2,
	       HDRST_URIDAT = 3,
	       HDRST_SPC2 = 4,
	       HDRST_OPTPARAM = 5,
	       HDRST_EOL1 = 6,
	       HDRST_KEY = 7,
	       HDRST_SPC3 = 8,
	       HDRST_VALUE = 9,
	       HDRST_EOL2 = 10 } hdrst_t;

typedef enum { HTTP_MTHD_NONE = 0,
	       HTTP_MTHD_POST = 1,
	       HTTP_MTHD_PUT = 2,
	       HTTP_MTHD_DELETE = 3,
               HTTP_MTHD_GET = 4 } http_method_t;

class methodmap_t {
public:
  methodmap_t ();
  http_method_t lookup (const str &s) const;
private:
  qhash<str,http_method_t> map;
};

extern methodmap_t methodmap;

class http_inhdr_t : public pairtab_t<> {
public:
  http_inhdr_t (abuf_t *a, cgi_t *u = NULL, cgi_t *c = NULL, 
		size_t bfln = HTTPHDR_DEF_SCRATCH, char *b = NULL)
    : abuf (a), uri (u), cookie (c), 
      buflen (min<size_t> (bfln, HTTPHDR_MAX_SCRATCH)), 
      scratch (b), scratchalloc (false), state (HDRST_START), hdrend (false),
      parsing (false), dataready (false), noins (false), nvers (0)
  {
    if (!scratch) {
      scratch = (char *) xmalloc (buflen);
      scratchalloc = true;
    }
    pcp = scratch;
    endp = scratch + buflen;
  }

  ~http_inhdr_t () 
  { 
    if (scratchalloc && scratch) xfree (scratch); 
  }

  void parse (cbi::ptr c);
  void can_read_cb ();
  void cancel ();
  inline str get_line1 () const { return line1; }
  inline str get_target () const { return target; }
  inline htpv_t get_vers () const { return nvers; }
  bool takes_gzip () const;

  http_method_t mthd;  // method code
  int reqsize;         // request size

private:
  inline bool iscookie () const;
  abuf_stat_t delimit_word (str *d, bool qms = false);
  abuf_stat_t delimit_key (str *k);
  abuf_stat_t delimit_val (str *v);
  bool eol () ;
  bool gobble_eol ();
  void _parse ();
  void ext_parse_cb ();
  void fixup ();

  abuf_t *abuf;
  cgi_t *uri;
  cgi_t *cookie;
  size_t buflen;
  char *scratch;
  bool scratchalloc;

  char *pcp;        // parse character pointer
  char *endp;       // end of the scratch buffer
  hdrst_t state;    // parse state
  str key, val;     // used in parsing key/val pairs
  str tmthd;        // POST, GET, etc...
  str target;       // URI given as target
  str vers;         // HTTP version
  bool hdrend;      // hit end-of-header

  cbi::ptr pcb;     // call this CB after parse is done
  bool parsing;     // flag is on if parsing
  bool dataready;   // on if we have data ready to read
  
  bool noins;       // on to disable insert into the pairtab (for Cookies)

  char scr2[SCR2_LEN]; // scratch for logging purposes, etc
  str line1;        // first line of the HTTP req
  htpv_t nvers;     // HTTP version; 0 ==> 1.0,  1 ==> 1.1
};


#endif /* _LIBAHTTP_INHDR_H */
