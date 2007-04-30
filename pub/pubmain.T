/* $Id$ */
// -*-c++-*-

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include <unistd.h>
#include "pub.h"
#include "pub2.h"
#include "pub_parse.h"
#include <stdlib.h>

static void
usage ()
{
  fatal << "usage: [-wr] [-g|-h|-e] [-F|-f<config>] [-o <outfile>] <infile>\n";
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    bool wss (false);
    bool iinfo  (true);
    bool readonly_out (false);
    pfile_type_t m (PFILE_TYPE_NONE);
    int ch;
    str infile, outfile;
    str configfile;
    str jaildir;
    bool nojail (false);
    bool noconfig (false);
    const char *e, *v;
    u_int opts (0);
    bool nodebug (false);
    bool evalmode (false);
    pub_parser_t *ppt;
    vec<str> cfgs;
    bool use_default_cfg (false);
    bhash<str> cfg_bmap;
    bool cfg_succ (false);
    bool ok;
    u_int i;
    int fd;
    bool err;
    ptr<pub2::local_publisher_t> lp;
    zbuf b;
  }

  // Initialize global pub variables
  zinit ();
  ppt = pub_parser_t::alloc ();

  if ((e = getenv ("PUBCONF")) && (v = getenvval (e)) && *v)
    configfile = v;
  
  while ((ch = getopt (argc, argv, "DFIcwrvheo:f:j:J")) != -1) {
    switch (ch) {
    case 'e':
      evalmode = true;
      break;
    case 'F':
      noconfig = true;
      break;
    case 'w':
      wss = true;
      break;
    case 'r':
      readonly_out = true;
      break;
    case 'I':
      iinfo = false;
      break;
    case 'f': 
     {
       str f = optarg;
       if (f == "-") {
	 use_default_cfg = true;
       } else if (cfg_bmap[f]) {
         warn << optarg << ": duplicate file; skipping\n";
       } else {
         cfg_bmap.insert (f); 
         cfgs.push_back (f);
       }
       break;
     }
    case 'h':
      if (m) usage ();
      m = PFILE_TYPE_H;
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

  if ((!noconfig && cfgs.size () == 0) || use_default_cfg) {
    str f = okws_etcfile (ok_pub_config);
    if (f)
      cfgs.push_back (f);
  }

  if (!nodebug)
    opts |= P_DEBUG;

  lp = New refcounted<pub2::local_publisher_t> (ppt, opts);

#ifdef PDEBUG
  yydebug = 1;
#endif /* PDEBUG */
  
  for (i = 0; i < cfgs.size (); i++) {
    twait { lp->run_cfg (cfgs[i], mkevent (ok)); }
    if (ok) {
      cfg_succ = true;
    }
  }
  
  if (configfile && !cfg_succ)
    warn << "pub running without default variable bindings\n";

  if (evalmode) {
    bool noisy = (optind < argc - 1);
    for (int i = optind; i < argc; i++) {
      str s = lp->cfg (argv[i]);
      if (noisy) printf ("(%s) -> ", argv[i]);
      if (s) printf ("%s", s.cstr ());
      if (noisy || (s && *s)) printf ("\n");
    }
    exit (0);
  }

  if (optind != argc - 1)
    usage ();
  infile = argv[optind];
  if (!outfile || !m) {
    pfile_type_t m2 = PFILE_TYPE_NONE;
    str out2;
    if ((out2 = suffix_sub (infile, ".shtml", ".html")) ||
	(out2 = suffix_sub (infile, ".pphp", ".php"))) {
      m2 = PFILE_TYPE_H;
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
    opts |= P_WSS;
  else if (iinfo)
    opts |= P_IINFO;
  if (m != PFILE_TYPE_H)
    opts |= P_COMPILE;
  
  // P_WSS needs to be a parser option, so reset the options here.
  ppt->set_opts (opts);
  lp->set_opts (opts);

  fd = myopen (outfile, readonly_out ? 0444 : 0644);
  if (fd < 0)
    usage ();
  err = false;

  twait { lp->run (&b, infile, mkevent (ok)); }
  if (!ok) {
    warn << infile << ": cannot open/parse file\n";
    err = true;
  } else {
    /*
    if (ppt->parseerr)
      err = true;
    */

    if (b.output (fd) < 0) {
      warn << outfile << ": cannot output\n";
      err = true;
    }
  }
  close (fd);
  if (err)
    unlink (outfile);
  exit (err ? 1 : 0);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
}
