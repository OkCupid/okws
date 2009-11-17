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

#include "pub3.h"
#include "pub3hilev.h"
#include "pub3parse.h"
#include <unistd.h>
#include <stdlib.h>
#include "parseopt.h"
#include "pslave.h"
#include "pubutil.h"
#include "aios.h"

//-----------------------------------------------------------------------

namespace pub3 {

  class eng_t {
  public:
    eng_t () : _opt (0), _ppt (NULL), _syntax_check (false) {}
    void init (int argc, char **argv, bool *gop, evi_t ev, CLOSURE);
    void run (evi_t ev, CLOSURE);
    void syntax_check (evi_t ev, CLOSURE);
  private:

    void run_pub (evi_t ev, CLOSURE);
    void check_files (evi_t ev, CLOSURE);
    void run_file (str s, evi_t ev, CLOSURE);
    void check_file (str s, evi_t ev, CLOSURE);

    int _opt;
    str _jaildir;
    str _config;
    ptr<pub3::pub_parser_t> _ppt;
    ptr<pub3::jailer_t> _jailer;
    ptr<pub3::local_publisher_t> _pub;
    vec<str> _files;
    str _argfile;
    pub3::obj_dict_t _dict;
    vec<str> _print_vars;
    bool _syntax_check;
  };

};

//-----------------------------------------------------------------------

tamed void
pub3::eng_t::check_file (str f, evi_t ev)
{
  tvars {
    int rc;
    str err;
  }
  twait { _pub->syntax_check (f, &err, mkevent (rc)); }
  if (rc != 0) {
    warn << err << "\n";
  }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed void
pub3::eng_t::run_file (str f, evi_t ev)
{
  tvars {
    bool ok;
    zbuf out;
  }
  twait { _pub->run (&out, f, mkevent (ok), _dict.dict (), _opt); }
  if (!ok) {
    warn << "** pub failed on file: " << f << "\n";
  } else {
    strbuf b;
    out.to_strbuf (&b, false);
    b.tosuio ()->output (1);
  }
  ev->trigger ( ok ? 0 : -1);
}

//-----------------------------------------------------------------------

tamed void
pub3::eng_t::check_files (evi_t ev)
{
  tvars {
    int frc (0); // final return code
    int trc (0); // temp return code
    size_t i;
  }
  
  for (i = 0; i < _files.size (); i++) {
    twait { check_file (_files[i], mkevent (trc)); }
    if (trc != 0) { frc = trc; }
  }
  ev->trigger (frc);
}

//-----------------------------------------------------------------------

tamed void
pub3::eng_t::run (evi_t ev)
{
  tvars {
    int rc;
  }
  if (_syntax_check) {
    twait { check_files (mkevent (rc)); }
  } else {
    twait { run_pub (mkevent (rc)); }
  }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed void
pub3::eng_t::run_pub (evi_t ev)
{
  tvars {
    int rc (0);
    size_t i;
    bool b (true);
  }

  if (_argfile) {
    twait { _pub->run_cfg (_argfile, mkevent (b), _dict.dict ()); }
  }

  if (!b) { 
    warn << "** failed to parse arguments file: " << _argfile << "\n";
    rc = -2;
  }

  for (i = 0; i < _files.size () && rc == 0; i++) {
    twait { run_file (_files[i], mkevent (rc)); }
  }

  for (i = 0; i < _print_vars.size () && rc == 0; i++) {
    str v = _pub->cfg (_print_vars [i]);
    if (!v) { v = "null"; }
    aout << _print_vars[i] << ": " << v << "\n";
  }

  ev->trigger (rc);
}

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: " << progname 
	<< " [-wevh] [-j <jaildir>] [-f <conf>] [-o <objfile>] [-a <argfile>] "
	<< " [-p <v1> -p <v2> .. ] "
	<< "<file-1> <file-2> ..\n"
	<< "\n"
	<< "    - Variables specified with -p will be printed after all files\n"
	<< "      are processed.\n"
	<< "\n"
	<< "    (Pub v3, OKWS" << OKWS_VERSION << ")\n";
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
    bool check_use_jail (false);
  }

  zinit ();
  set_debug_flags ();

  _ppt = pub3::pub_parser_t::alloc ();

  if ((e = getenv ("PUBCONF")) && (v = getenvval (e)) && *v) {
    _config = v;
  }

  while ((ch = getopt (argc, argv, "a:diwej:f:vhp:sJ")) != -1) {
    switch (ch) {
    case 's':
      _syntax_check = true;
      break;
    case 'p':
      _print_vars.push_back (optarg);
      break;
    case 'a':
      _argfile = optarg;
      break;
    case 'i':
      _opt |= P_IINFO;
      break;
    case 'd':
      _opt |= P_DEBUG;
      break;
    case 'w':
      _opt |= P_WSS; 
      break;
    case 'e':
      _opt |= P_VISERR;
      break;
    case 'j':
      _jaildir = optarg;
      break;
    case 'J':
      check_use_jail = true;
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

  if (!run) {
    /* noop */
  } else {
    argc -= optind;
    argv += optind;

    for (int i = 0; i < argc; i++) {
      _files.push_back (argv[i]);
    }

    if (_syntax_check) {
      if (_print_vars.size ()) {
	warn << "XX print vars have no effect in syntax-check mode\n";
      }
      if (_opt) {
	warn << "XX pub optinos have no effect in syntax-check mode\n";
      }
    }
    
    if (!_config) {
      _config = okws_etcfile (ok_pub_config);
    }
    
    _pub = New refcounted<pub3::local_publisher_t> (_ppt, _opt);
    
    have_cfg = false;
    if (_config) {
      twait { _pub->run_cfg (_config, mkevent (have_cfg)); }
    }
    
    if (!have_cfg && !_syntax_check)  {
      warn << "pub running without default variable bindings\n";
    }
    
    if (!_jaildir && (!_syntax_check || check_use_jail)) {
      if (!_pub->cfg ("JailDir", &_jaildir, true)) {
	warn << "XX cannot find a jail directory!!\n";
	warn << "XX   - none was specified with -j and "
	     << "no 'JailDir' found in config file.\n";
      }
    }
    
    if (!_jaildir) {
      /* issues ahead, maybe... */
    } else if (stat (_jaildir.cstr (), &sb) != 0) {
      warn << "XX no directory found for JailDir: " << _jaildir << "\n";
      _jaildir = NULL;
    } else if (!S_ISDIR(sb.st_mode)) {
      warn << "XX supposed 'JailDir' is not a directory: " << _jaildir << "\n";
      _jaildir = NULL;
    }

    if (_syntax_check) {
      /* skip next checks .. */
    } else if (!_jaildir) {
      run = false;
      rc = -1;
    } else if (_files.size () == 0 && _print_vars.size () == 0) {
      run = false;
      rc = -1;
      warn << "XX no files given! need at least one\n";
      usage ();
    }

    if (rc == 0 && _jaildir) {
      _ppt->setjail_virtual (_jaildir);
    }

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
