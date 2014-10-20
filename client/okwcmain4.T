// -*-c++-*-

/* $Id: okwcmain.C 1635 2006-03-16 17:28:10Z max $ */

#include "async.h"
#include "parseopt.h"
#include "vec.h"
#include "okwc.h"
#include "aios.h"
#include "parseopt.h"
#include "tame.h"
#include "okwc4.h"
#include "tame_connectors.h"
#include "oksslutil.h"


//-----------------------------------------------------------------------

void
usage ()
{
  warn << "usage: okwc [-k] [-t <timeout>] [-o <status>] [-p <proxy>] "
       << " [ -f <post-file> ] "
       << "<url1> <url2> ..\n";
  exit (1);
}

//-----------------------------------------------------------------------

class my_resp_t : public okwc4::resp_simple_t {
public:
  my_resp_t (ptr<ok_xprt_base_t> x, ptr<abuf_t> a, vec<int> s) 
    : okwc4::resp_simple_t (x,a) 
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

class my_factory_t : public okwc4::obj_factory_t {
public:
  my_factory_t (vec<int> s) : _ok_status (s) {}

  ptr<okwc4::resp_t> alloc_resp (ptr<ok_xprt_base_t> x, ptr<abuf_t> a) 
  { return New refcounted<my_resp_t> (x, a, _ok_status); }

private:
  vec<int> _ok_status;
};

//-----------------------------------------------------------------------

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    str post;
    ptr<okwc4::agent_get_t> cli;
    vec<ptr<okwc4::resp_t> > respv;
    vec<int> statusv;
    str prx;
    int ch;
    int timeout (-1);
    vec<str> extra_headers;
    vec<int> ok_status;
    int tmp;
    ptr<my_factory_t> factory;
    ptr<okwc4::reqargs_t> ra, pra;
    vec<ptr<okwc4::reqargs_t> > rav;
    str postfile;
    size_t s;
    bool ka (false);
    rendezvous_t<> rv (__FILE__, __LINE__);
    bool parallel (false);
  }

  okssl::init_ssl_internals ();
  okwc_def_contlen *= 10;

  while ((ch = getopt (argc, argv, "t:p:h:o:f:kl")) != -1) {
    switch (ch) {
    case 'l':
      parallel = true;
      break;
    case 'k':
      ka = true;
      break;
    case 'f':
      postfile = optarg;
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

  if (!prx)
    prx = getenv ("http_proxy");

  if (argc < 1) usage ();

  for (int i = 0; i < argc; i++) {
    const char *hn = argv[i];
    if (prx) {
      ra = okwc4::reqargs_t::alloc_proxied (hn, prx);
    } else {
      ra = okwc4::reqargs_t::alloc (hn);
    }
    if (!ra) {
      warn << "Cannot parse hostname: " <<  hn;
      if (prx) { warnx << " (proxy=" << prx << ")"; }
      warnx << "\n";
    } else {
      rav.push_back (ra);
    }
  }

  if (postfile) {
    if (!(post = file2str (postfile))) {
      warn ("Cannot open post file %s: %m\n", postfile.cstr ());
      usage ();
    }
  }

  respv.setsize (rav.size ());
  statusv.setsize (rav.size ());
  for (s = 0; s < rav.size (); s++) {
    ra = rav[s];
    if (!cli || !pra || *(pra->host ()) != *(ra->host ()) ) {
      cli = New refcounted<okwc4::agent_get_t> (ra->host ());
    }
    cli->set_keepalive (ka);
    statusv[s] = HTTP_TIMEOUT;
    cli->get (ra, connector::timeout (mkevent (rv, statusv[s], respv[s]), timeout));
    pra = ra;

    if (!parallel) {
      twait (rv);
    }
  }
  
  while (rv.need_wait ()) {
    twait (rv);
  }

  for (s = 0; s < rav.size (); s++) {
    int status = statusv[s];
    ptr<okwc4::resp_t> resp = respv[s];
    
    aout << "status(" << s << "): " << status << "\n";
    if (status == HTTP_MOVEDTEMP || status == HTTP_MOVEDPERM) {
      aout << "Redirect to: " << (* resp->hdr () )["location"] << "\n";
    } else if (resp && resp->body ()) {
      aout << "\nbody(" << s << "): " << resp->body () << "\n";
    }
  }

  // delay to find any bugz
  twait { delaycb (2, 0, mkevent ()); }

  exit (0);
}

//-----------------------------------------------------------------------

int 
main (int argc, char *argv [])
{
  set_debug_flags ();
  main2 (argc, argv);
  amain ();
}

//-----------------------------------------------------------------------
