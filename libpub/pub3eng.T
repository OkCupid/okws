// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#include "pub3.h"
#include "pub3hilev.h"
#include "pub3parse.h"
#include <unistd.h>
#include <stdlib.h>
#include "parseopt.h"
#include "pslave.h"
#include "pubutil.h"
#include "aios.h"
#include "pub3eng.h"

//-----------------------------------------------------------------------

tamed void
pub3::eng_t::check_file (str f, evi_t ev)
{
  tvars {
    int rc;
    vec<str> err;
  }
  twait { _pub->syntax_check (f, &err, mkevent (rc)); }
  if (rc != 0) {
    for (size_t i = 0; i < err.size (); i++) {
      warn << err[i] << "\n";
    }
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
    pub3::obj_t uo;
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

  uo = _pub->get_universals_obj ();

  for (i = 0; i < _print_vars.size () && rc == 0; i++) {
    
    str v = uo (_print_vars [i]).to_str ();
    if (!v) { v = "null"; }
    aout << _print_vars[i] << ": " << v << "\n";
  }

  ev->trigger (rc);
}

//-----------------------------------------------------------------------

void
pub3::eng_t::usage ()
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
    bool check_use_jail (true);
    pub3::obj_dict_t tmp_d;
    pub3::opts_t cmd_opt (0);
  }

  zinit ();
  set_debug_flags ();

  _ppt = pub3::pub_parser_t::alloc ();

  if ((e = getenv ("PUBCONF")) && (v = getenvval (e)) && *v) {
    _config = v;
  }

  while ((ch = getopt (argc, argv, "a:diej:f:vhp:sJ")) != -1) {
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
      cmd_opt |= P_IINFO;
      break;
    case 'd':
      cmd_opt |= P_DEBUG;
      break;
    case 'e':
      cmd_opt |= P_VISERR;
      break;
    case 'j':
      _jaildir = optarg;
      break;
    case 'J':
      check_use_jail = false;
      break;
    case 'f':
      _config = optarg;
      break;
    case 'v':
      cmd_opt |= P_VERBOSE;
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

  // OR in the command opts
  _opt |= cmd_opt;

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
      if (cmd_opt) {
	warn << "XX pub options have no effect in syntax-check mode\n";
      }
    }
    
    if (!_config) {
      _config = okws_etcfile (ok_pub_config);
    }
    
    _pub = New refcounted<pub3::local_publisher_t> (_ppt, _opt);
    
    have_cfg = false;
    if (_config) {
      twait { _pub->run_cfg (_config, mkevent (have_cfg), tmp_d.dict ()); }
    }
    
    if (!have_cfg && !_syntax_check)  {
      warn << "pub running without default variable bindings\n";
    }
    
    if (!_jaildir && !_syntax_check && check_use_jail) {
      if (!tmp_d("JailDir").to_str (&_jaildir)) {
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
    } else if (!_jaildir && check_use_jail) {
      run = false;
      rc = -1;
    } else if (_files.size () == 0 && _print_vars.size () == 0) {
      run = false;
      rc = -1;
      warn << "XX no files given! need at least one\n";
      usage ();
    }

    if (rc == 0 && _jaildir) {
      _pub->jailer ()->setjail (JAIL_VIRTUAL, _jaildir);
    }

  }
  
  *gop = run;
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed void
pub3::eng_t::main (int argc, char **argv, evi_t ev)
{
  tvars {
    int rc (0);
    bool go (false);
  }

  twait { init (argc, argv, &go, mkevent (rc)); }
  if (go) { twait { run (mkevent (rc)); } }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------
