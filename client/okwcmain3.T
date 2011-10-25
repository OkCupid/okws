// -*-c++-*-

/* $Id: okwcmain.C 1635 2006-03-16 17:28:10Z max $ */

#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "okwc.h"
#include "aios.h"
#include "parseopt.h"
#include "rxx.h"
#include "tame.h"
#include "okwc3.h"
#include "tame_connectors.h"
#include "oksslutil.h"

static rxx url_rxx ("http(s)?://([^:/]+)(:(\\d+))?(/(.*))?");

void
usage ()
{
  warn << "usage: okwc [-c] [-t <timeout>] [-o <status>] [-p <proxy>] "
       << "[-n <n-attempts>] <url> [<post-file>]\n";
  exit (1);
}

//-----------------------------------------------------------------------

class my_resp_t : public okwc3::resp_simple_t {
public:
  my_resp_t (ptr<ok_xprt_base_t> x, ptr<abuf_t> a, vec<int> s) 
    : okwc3::resp_simple_t (x, a) 
  {
    _ok_status.insert (HTTP_OK);
    for (size_t i = 0; i < s.size () ; i++) {
      warn << "Add: " << s[i] <<"\n";
      _ok_status.insert (s[i]);
    }
  }

  bool do_get_body (int s) const { return _ok_status[s]; }
  bhash<int> _ok_status;
};

//-----------------------------------------------------------------------

class my_factory_t : public okwc3::obj_factory_t {
public:
  my_factory_t (vec<int> s) : _ok_status (s) {}

  ptr<okwc3::resp_t> alloc_resp (ptr<ok_xprt_base_t> x, ptr<abuf_t> a) 
  { return New refcounted<my_resp_t> (x, a, _ok_status); }

private:
  vec<int> _ok_status;
};

//-----------------------------------------------------------------------

static str 
my_join (const vec<str> &v, str sep)
{
  strbuf b;
  for (size_t i = 0; i < v.size (); i++) {
    if (i) { b << sep; }
    b << v[i];
  }
  return b;
}


//-----------------------------------------------------------------------

tamed static void
main3 (int argc, char **argv, evi_t ev)
{
  tvars {
    str post;
    str typ;
    str hostname;
    u_int16_t port (80);
    str port_str;
    str filename;
    ptr<okwc3::agent_get_t> cli;
    ptr<okwc3::resp_t> resp;
    int status (HTTP_TIMEOUT);
    str prx;
    str connect_to;
    int ch;
    int timeout (-1);
    bool https (false);
    vec<str> extra_headers;
    vec<int> ok_status;
    int tmp;
    ptr<my_factory_t> factory;
    bool show_cookies (false);
    size_t i (0), n (1);
  }

  okssl::init_ssl_internals ();
  okwc_def_contlen *= 10;

  while ((ch = getopt (argc, argv, "ct:p:h:o:n:")) != -1) {
    switch (ch) {
    case 'c':
      show_cookies = true;
      break;
    case 't':
      if (!convertint (optarg, &timeout))
	usage ();
      break;
    case 'p':
      prx = optarg;
      break;
    case 'h':
      extra_headers.push_back (optarg);
      break;
    case 'n':
      if (!convertint (optarg, &n)) {
	warn << "bad number of attempts: " << n << "\n";
	usage ();
      }
      break;
    case 'o':
      if (!convertint (optarg, &tmp)) {
	warn << "Bad status " << optarg << ": must be an int!\n";
	usage ();
      }
      ok_status.push_back (tmp);
      break;
    default:
      usage ();
    }
  }

  factory = New refcounted<my_factory_t> (ok_status);

  argc -= optind;
  argv += optind;


  if (argc != 1 && argc != 2) 
    usage ();

  if (!prx)
    prx = getenv ("http_proxy");

  if (prx) {
    filename = argv[0];
    connect_to = prx;
  } else {
    connect_to = argv[0];
  }

  if (!url_rxx.match (connect_to))
    usage ();

  if (url_rxx[1] && url_rxx[1] == "s") {
    https = true;
  }

  if (argc == 2) {
    str file = argv[1];
    if (!(post = file2str (file))) {
      warn ("Cannot open post file %s: %m\n", file.cstr ());
      usage ();
    }
    typ = "application/x-www-form-urlencoded";
  }

  hostname = url_rxx[2];
  port = https ? 443 : 80;
  port_str = url_rxx[4];

  if (port_str && port_str.len ()) {
    bool rc = convertint (port_str, &port);
    assert (rc);
  }

  if (prx) {
    cli = New refcounted<okwc3::agent_get_proxied_t> (hostname, port, https, 
						      factory);
  } else {
    filename = url_rxx[6];
    cli = New refcounted<okwc3::agent_get_direct_t> (hostname, port, https,
						     factory);
  }
  
  for (i = 0; i < n; i++) {

    twait { 
      cli->get (filename, 
		connector::timeout (mkevent (status, resp), timeout),
		1, NULL, 
		post, &extra_headers);
    }
    
    aout << "status: " << status << "\n";
    if (status == HTTP_MOVEDTEMP || status == HTTP_MOVEDPERM) {
      aout << "Redirect to: " << (* resp->hdr () )["location"] << "\n";
    } else if (resp) { 
      if (resp->body ()) {
	aout << "\nbody: " << resp->body () << "\n";
      } 
      const okwc_cookie_set_t &cs = resp->incookies ();
      for (size_t i = 0; show_cookies && i < cs.size (); i++) {
	const cgi_t *c = cs[i];
	aout << "\nInCookie[" << i << "]:\n";
	for (const pair_t *p = c->lfirst (); p; p = c->lnext (p)) {
	  str s = my_join (p->vals, ":");
	  aout << "   " << p->key << " -> " << s << "\n";
	}
      }
    }
  }
  ev->trigger (0);
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    int rc; 
  }
  twait { main3 (argc, argv, mkevent (rc)); }
  twait { delaycb (1, 0, mkevent ()); }
  exit (rc);
}

int 
main (int argc, char *argv [])
{
  set_debug_flags ();
  main2 (argc, argv);
  amain ();
}

