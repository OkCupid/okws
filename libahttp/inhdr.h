

// -*-c++-*-
/* $Id$ */

#ifndef _LIBAHTTP_INHDR_H
#define _LIBAHTTP_INHDR_H

#include "hdr.h"
#include "cgi.h"
#include "qhash.h"

typedef enum { INHDRST_START = 0,
	       INHDRST_SPC1 = 1,
	       INHDRST_TARGET = 2,
	       INHDRST_URIDAT = 3,
	       INHDRST_SPC2 = 4,
	       INHDRST_OPTPARAM = 5,
	       INHDRST_EOL1 = 6,
	       INHDRST_KEY = 7,
	       INHDRST_SPC3 = 8,
	       INHDRST_VALUE = 9,
	       INHDRST_EOL2 = 10 } inhdrst_t;

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

class http_inhdr_t : public http_hdr_t {
public:
  http_inhdr_t (abuf_t *a, cgi_t *u = NULL, cgi_t *c = NULL, 
		size_t bfln = HTTPHDR_DEF_SCRATCH, char *b = NULL)
    : http_hdr_t (a, bfln, b), 
      uri (u), cookie (c), state (INHDRST_START)
  {}

  inline str get_line1 () const { return line1; }
  inline str get_target () const { return target; }
  bool takes_gzip () const;
  inline str get_mthd_str () const { return tmthd; }
  inline str get_vers_str () const { return vers; }

  http_method_t mthd;  // method code

protected:
  virtual void _parse ();
  virtual void ext_parse_cb ();
  virtual void fixup ();

  cgi_t *uri;
  cgi_t *cookie;
  inhdrst_t state;    // parse state

  str tmthd;        // POST, GET, etc...
  str target;       // URI given as target
  str vers;         // HTTP version
  str line1;        // first line of the HTTP req
};


#endif /* _LIBAHTTP_INHDR_H */
