
#include <unistd.h>
#include "pub.h"
#include "pub_parse.h"
#include <stdlib.h>

static void
usage ()
{
  fatal << "usage: [-wrF] [-g | -h | -e] [-o <outfile>] <infile>\n";
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  bool wss = false;
  bool iinfo  = true;
  bool readonly_out = false;
  pfile_type_t m = PFILE_TYPE_NONE;
  int ch;
  str infile, outfile;
  str configfile;
  str jaildir;
  bool nojail = false;
  bool noconfig = false;
  const char *e, *v;
  u_int opts = 0;
  bool nodebug = false;
  // setup global gzip table
  zinit ();

  // Initialize global pub variables
  pub_parser_t *ppt = pub_parser_t::alloc ();

  setprogname (argv[0]);

  if ((e = getenv ("PUBCONF")) && (v = getenvval (e)) && *v)
    configfile = v;
  
  while ((ch = getopt (argc, argv, "DFIcwrgvheo:f:j:J")) != -1) {
    switch (ch) {
    case 'F':
      noconfig = true;
      break;
    case 'w':
      wss = true;
      break;
    case 'r':
      readonly_out = true;
      break;
    case 'g':
      if (!m) usage ();
      m = PFILE_TYPE_CODE;
    case 'I':
      iinfo = false;
      break;
    case 'f':
      configfile = optarg;
      break;
    case 'h':
      if (!m) usage ();
      m = PFILE_TYPE_H;
      break;
    case 'e':
      if (!m) usage ();
      m = PFILE_TYPE_EC;
      break;
    case 'o':
      outfile = optarg;
      break;
    case 'j':
      jaildir = optarg;
      break;
    case 'J':
      nojail = true;
      break;
    case 'D':
      nodebug = true;
      break;
    case 'v':
      opts |= P_VERBOSE;
      break;
    default:
      usage ();
    }
  }
  if (noconfig && configfile)
    usage ();
  if (jaildir && nojail)
    usage ();
  if (!noconfig && !configfile)
    configfile = ok_pub_config;

  if (!nodebug)
    opts |= P_DEBUG;
  ppt->set_opts (opts);

#ifdef PDEBUG
  yydebug = 1;
#endif /* PDEBUG */

  if (configfile && !ppt->parse_config (configfile)) 
    warn << "pub running without default variable bindings\n";


  if (optind != argc - 1)
    usage ();
  infile = argv[optind];
  if (!outfile || !m) {
    pfile_type_t m2 = PFILE_TYPE_NONE;
    str out2;
    if ((out2 = suffix_sub (infile, ".shtml", ".html"))) {
      m2 = PFILE_TYPE_H;
    } else if ((out2 = suffix_sub (infile, ".g", ".C")) ||
	       (out2 = suffix_sub (infile, ".ok", ".C"))) {
      nojail = true;
      m2 = PFILE_TYPE_CODE;
    } else if ((out2 = suffix_sub (infile, ".ec", ".C"))) {
      m2 = PFILE_TYPE_EC;
    }
    if (!m) m = m2;
    if (!outfile) outfile = out2;
  }

  if (!nojail)
    ppt->setjail (jaildir, true);

  if (!m) {
    warn << "Could not determine file type\n";
    usage ();
  }
  if (!outfile) {
    warn << "Could not determine outfile name\n";
    usage ();
  }

  if (wss) 
    ppt->wss = true;
  else if (iinfo)
    opts |= P_IINFO;
  if (m != PFILE_TYPE_H)
    opts |= P_COMPILE;

  int fd = myopen (outfile, readonly_out ? 0444 : 0644);
  if (fd < 0)
    usage ();
  bool err = false;

  bpfcp_t bnd;
  const pbinding_t *i;

  if (!(i = ppt->to_binding (infile, NULL, true))) {
    warn << infile << ": cannot open file\n";
    err = true;
  } else if (!(bnd = ppt->parse (i, m))) {
    warn << infile << ": bailing out due to parse errors\n";
    err = true;
  } else {
    /*
    if (ppt->parseerr)
      err = true;
    */
#ifdef PDEBUG 
    dumper_t d;
    bnd->dump (&d);
#endif /* PDEBUG */

    /*
     * debug code
     *
    xpub_file_t x;
    bnd->file->to_xdr (&x);
    strbuf bbb;
    rpc_print (bbb, x);
    warn << "FOO: " << bbb << "\n";
    */

    zbuf b;
    if (!ppt->include (&b, bnd, opts) || (b.output (fd) < 0))
      err = true;
  }
  close (fd);
  if (err)
    unlink (outfile);
  return (err ? 1 : 0);
}
