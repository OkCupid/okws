// -*-c++-*-
/* $Id: pubd.T 4245 2009-03-24 00:11:42Z max $ */

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

#include "pub.h"
#include "pub_parse.h"
#include "xpub.h"
#include <unistd.h>
#include <stdlib.h>
#include "pubd.h"
#include "parseopt.h"
#include "pslave.h"
#include "pubd2.h"
#include "pubutil.h"
#include "okdbg.h"
#include "pub2.h"
#include "pub3.h"

//-----------------------------------------------------------------------

namespace pub3 {

  class eng_t {
  public:
    eng_t () : _opt (0), _ppt (NULL) {}
    void init (int argc, char **argv, bool *gop, evi_t ev, CLOSURE);
    void run (evi_t ev, CLOSURE);
  private:
    int _opt;
    str _jaildir;
    str _config;
    pub_parser_t *_ppt;
    ptr<pub2::local_publisher_t> _cfgparse;
  };

};

//-----------------------------------------------------------------------

tamed void
pub3::eng_t::run (evi_t ev)
{
  ev->trigger (0);
}

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: " << progname 
	<< " [-wevh] [-j <jaildir>] [-f <conf>] [-o <objfile>] "
	<< "<file-1> <file-2> ..\n";
}


//-----------------------------------------------------------------------

tamed void
pub3::eng_t::init (int argc, char **argv, bool *gop, evi_t ev)
{
  tvars {
    bool run (true);
    int rc;
    int ch;
    const char *e, *v;
    bool have_cfg;
    struct stat sb;
  }

  zinit ();
  set_debug_flags ();

  _ppt = pub_parser_t::alloc (true);

  if ((e = getenv ("PUBCONF")) && (v = getenvval (e)) && *v) {
    _config = v;
  }

  while ((ch = getopt (argc, argv, "wej:f:vh")) != -1) {
    switch (ch) {
    case 'w':
      _opt |= P_WSS; 
      break;
    case 'e':
      _opt |= P_VISERR;
      break;
    case 'j':
      _jaildir = optarg;
      break;
    case 'f':
      _config = optarg;
      break;
    case 'v':
      _opt |= P_VERBOSE;
      break;
    case 'h':
      usage ();
      run = false;
      break;
    default:
      usage ();
      run = false;
      rc = -2;
      break;
    }
  }

  if (!_config) {
    _config = okws_etcfile (_config);
  }

  _cfgparse = New refcounted<pub2::local_publisher_t> (_ppt, _opt);

  have_cfg = false;
  if (_config) {
    twait { _cfgparse->run_cfg (_config, mkevent (have_cfg)); }
  }

  if (!have_cfg)  {
    warn << "pubd running without default variable bindings\n";
  }

  if (!_jaildir) {
    if (!_cfgparse->cfg ("JailDir", &_jaildir, true)) {
      warn << "Cannot find a jail directory; none was specified with -j\n"
	   << "  and no 'JailDir' found in config file.\n";
    }
  }

  if (!_jaildir) {
    /* issues ahead! */
  } else if (stat (_jaildir.cstr (), &sb) != 0) {
    warn << "No directory found for JailDir: " << _jaildir << "\n";
    _jaildir = NULL;
  } else if (!S_ISDIR(sb.st_mode)) {
    warn << "Supposed 'JailDir' is not a directory: " << _jaildir << "\n";
    _jaildir = NULL;
  }

  if (!_jaildir) {
    run = false;
    rc = -1;
  } 

  *gop = run;
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    int rc (0);
    bool go;
    pub3::eng_t eng;
  }

  twait { eng.init (argc, argv, &go, mkevent (rc)); }
  if (go) {
    twait { eng.run (mkevent (rc)); }
  }
  exit (rc);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
}

//-----------------------------------------------------------------------
