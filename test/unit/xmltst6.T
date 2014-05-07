// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxml.h"
#include "abuf_pipe.h"
#include "okxmlparse.h"
#include "okxmlgeneric.h"
#include "okxmlobj.h"
#include "tame.h"
#include "parseopt.h"
#include <sys/resource.h>

//
// Caching costs:
//
//   n=1000 => 89004 virt, 80m RES, 6092 SHR
//   n=2000 => 160m virt, 153 RES, 6092 SHR
//   n=3000 => 233m virt 226 RES, 6092 SHR

static void usage (void)
{
  warnx << "usage: " << progname << "<n-times>\n";
  exit (1);
}

static bool memtest;

tamed static void
read_doc (evs_t cb)
{
  tvars {
    vec<str> v;
    str s;
    int err;
    bool go (true);
    strbuf buf;
  }

  while (go) {
    twait { ain->readany (mkevent (s, err)); }
    if (err) {
      warn << "Read error: " << err << "\n";
      go = false;
    } else if (!s) {
      go = false; // EOF
    } else {
      v.push_back (s);
      buf << s;
    }
  }

  {
    str s (buf);
    if (s) {
      warn << "Read in doc; " << s.len () << " bytes long\n";
    } else {
      warn << "Failed to read in doc\n";
    }
    cb->trigger (s);
  }

}

tamed static void
parse_doc (str s, xml_gobj_t *res, evb_t cb)
{
  tvars {
    abuf_str_t src (s);
    ptr<xml_req_parser_generic_t> parser;
    int prc;
    bool rc (false);
  }
  parser = New refcounted<xml_req_parser_generic_t> (&src);
  parser->init ();
  twait { parser->parse (mkevent (prc)); }
  prc = parser->errcode ();
  if (prc == XML_PARSE_OK) {
    *res = xml_gobj_t (parser->top_level_g ());
    rc = true;
  } else {
    warn << "parse failure: " << prc << "\n";
  }
  cb->trigger (rc);
}

tamed static void
main_T (int n)
{
  tvars {
    str s;
    xml_gobj_t o;
    vec<xml_gobj_t> v;
    int i;
    bool ok;
  }
  twait { read_doc (mkevent (s)); }
  if (s) {
    for (i = 0; i < n; i++) {
      if (i % 10 == 0)
	warn << "iter " << i << "\n";
      twait { parse_doc (s, &o, mkevent (ok)); }
      if (ok) {
	str s = o("OkTest")("Var")[0]("Name").data ().trim ();
	if (memtest)
	  v.push_back (o); 
      }
    }
  }

  if (memtest) {
    struct rusage ru;
    int rc = getrusage (RUSAGE_SELF, &ru);
    if (rc == 0) {
      warn << "rusage: rss=" << ru.ru_maxrss << "; "
	   << "ixrss=" << ru.ru_ixrss << "; "
	   << "idrss=" << ru.ru_idrss << "\n";
    } else {
      warn ("rusage: %m\n");
    }
    warn << "busy loop!\n";
    while (1) {}
  }
  exit (0);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  if (argc != 2 && argc != 1) 
    usage ();

  memtest = getenv ("MEMTEST");

  int n = 1;
  if (argc == 2 && !convertint (argv[1], &n))
    usage ();
    
  main_T (n);
  amain ();

}
