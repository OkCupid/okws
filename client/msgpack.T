// -*-c++-*-

#include "pub3parse.h"
#include "pub3msgpack.h"

//-----------------------------------------------------------------------

static str
consume_stdin () 
{
#define BUFLEN 1024

  char buf[BUFLEN];
  int rc;
  strbuf b;
  vec<str> v;
  str ret;

  make_sync (0);

  while ((rc = read (0, buf, BUFLEN)) > 0) {
    str s (buf, rc);
    b << s;
    v.push_back (s);
  }
  
  if (rc < 0) {
    warn ("read error: %m\n");
    ret = NULL;
  } else {
    ret = b;
  }
  return ret;

#undef BUFLEN
}

//-----------------------------------------------------------------------

static void usage (void)
{
  warnx << "usage: " << progname << " [-de]\n"
	<< "\n"
	<< "    Pass in input on standard input, specify -d for\n"
	<< "    decode and -e for encode.  It encodes by default!\n";
  ;
  exit (1);
}


//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);

  int ch;
  bool encode = true;
  while ((ch = getopt (argc, argv, "de")) != -1) {
    switch (ch) {
    case 'd': encode = false; break;
    case 'e': encode = true; break;
    default:
      usage ();
    }
  }

  str s = consume_stdin ();
  str res;
  if (encode) {
    ptr<pub3::expr_t> e = pub3::json_parser_t::parse (s);
    if (!e) {
      warn << "parse of JSON on stdin failed\n";
    } else {
      res = pub3::msgpack::encode (e);
      if (!res) {
	warn << "Encoding to msgpack failed!\n";
      }
    }
  } else {
    ptr<pub3::expr_t> e = pub3::msgpack::decode (s);
    if (!e) {
      warn << "decode of msgpack failed\n";
    } else {
      res = e->to_str ();
    }
  }

  if (!res) {
    exit (-1);
  }

  make_sync (1);
  strbuf b;
  b << res;
  b.tosuio ()->output (1);
  exit (0);
}

#undef BUFLEN
