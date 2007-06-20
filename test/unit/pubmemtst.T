
//-*-c++-*-

#include "pub.h"
#include "pub2.h"
#include "parseopt.h"

void
usage ()
{
  warn << "usage: " << progname << " [-i <n-iter>] [-f <file>] <N=V> <N=V>\n";
  exit (1);
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    pub_parser_t *ppt;
    int ch;
    int n_iter (100), n_iter_copy_aarr (0), n_elem (100);
    aarr_t aarr;
    str file;
    int i;
    ptr<pub2::local_publisher_t> lp;
    u_int opts (0);
    ptr<zbuf> zb;
    bool ok;
  }
  static rxx a_eq_b ("([^=\\s]+)=(\\S*)");

  zinit ();
  ppt = pub_parser_t::alloc ();

  while ((ch = getopt (argc, argv, "i:f:c:e:")) != -1)
    switch (ch) {
    case 'c':
      if (!convertint (optarg, &n_iter_copy_aarr))
	usage ();
      break;
    case 'i':
      if (!convertint (optarg, &n_iter))
	usage ();
      break;
    case 'e':
      if (!convertint (optarg, &n_elem))
	usage ();
      break;
    case 'f':
      file = optarg;
      break;
    default:
      usage ();
    }

  argc -= optind;
  argv += optind;

  if (!file && n_iter > 0) {
    warn << "No file specified\n";
    usage ();
  }

  for (i = 0; i < argc; i++) {
    if (!a_eq_b.match (argv[i]))
      usage ();
    aarr.add (a_eq_b[1], a_eq_b[2]);
  }

  lp = New refcounted<pub2::local_publisher_t> (ppt, opts);

  for (i = 0; i < n_iter; i++) {
    if (i % 100 == 0) {
      warn << "Iter: " << i << "\n";
    }
    zb = New refcounted<zbuf> ();
    twait { lp->run (zb, file, mkevent (ok)); }
    if (!ok) {
      warn << "Pub of file failed: " << file << "\n";
    }
  }

  for (i = 0; i < n_iter_copy_aarr; i++) {
    if (i % 100 == 0) {
      warn << "Iter: " << i << "\n";
    }
    aarr_t n;
    for (int j = 0; j < n_elem; j++) {
      strbuf k("k%d.%d", i, j);
      strbuf v("v%d.%d", i, j);
      n.add (k, v);
    }
    aarr = n;
  }

  exit (0);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
}

