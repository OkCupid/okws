//-*-c++-*-

/* $Id$ */
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

/**
 * treestat
 * 
 *  A smallish program that recursively explores a file system tree,
 *  starting with a given directory.  Checks to see if any files in the
 *  tree have changed since the last exploration; if yes, then touch
 *  a "sentinel file."  If not, just touch a heartbeat file.
 *
 */
#include "ihash.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "pubutil.h"
#include "parseopt.h"
#include "pub.h"
#include "pub2.h"

enum { D_NO = 0, D_LOW = 1, D_MED = 2, D_HI = 3};

struct fid_t {
  fid_t (const struct stat &s) :
    _dev (s.st_dev), _ino (s.st_ino), _hshkey (_dev * _ino) {}
  fid_t () {}

  operator hash_t () const { return _hshkey; }
  bool operator== (const fid_t &f2) const 
  { return (_dev == f2._dev && _ino == f2._ino); }
    
  dev_t _dev;
  ino_t _ino;
  hash_t _hshkey;
};


struct file1_t {
  file1_t (const str &f, int m) : _fn (f), _mode (m) {}

  bool init () {
    if (!touch ())
      return false;

    struct stat sb;
    if (stat (_fn.cstr (), &sb) < 0) {
      warn ("Cannot open file %s: %m\n", _fn.cstr ());
      return false;
    }
    _fid = fid_t (sb);

    return true;
  }
  const fid_t &fid () const { return _fid; }

  bool touch ()
  {
    int fd = open (_fn.cstr (), O_WRONLY|O_CREAT|O_TRUNC, _mode);
    if (fd < 0) {
      warn ("Cannot open file %s for writing: %m\n", _fn.cstr ());
      return false;
    }
    strbuf b;
    b << time (NULL);
    if (b.tosuio ()->output (fd) < 0) {
      warn ("Write to file %s failed: %m\n", _fn.cstr ());
      close (fd);
      return false;
    }
    close (fd);
    return true;
  } 
  
  const str &fn () const { return _fn ; }

  str _fn;
  fid_t _fid;
  int _mode;
};

struct file2_t {
  file2_t () : _mark_bit (false), _visited (false) {}
  file2_t (const str &fn, const fid_t &f, const time_t &c) 
    : _fn (fn), _fid (f), _ctime (c), _mark_bit (false) {}
  str _fn;
  fid_t _fid;
  time_t _ctime;
  ihash_entry<file2_t> _lnk;
  bool _mark_bit;
  bool _visited;
};

typedef ihash<fid_t, file2_t, &file2_t::_fid, &file2_t::_lnk> filetab_t;

struct treestat_t {
  treestat_t (const str &d, const str &s, const str &h, int i, int v, int m)
    : _dir (d), 
      _sentinel (s, m), 
      _heartbeat (h, m),
      _check_interval (i),
      _verbose (v) {}

  bool init () {
    if (!_sentinel.init () || !_heartbeat.init ())
      return false;
    _skiptab.insert (_sentinel.fid ());
    _skiptab.insert (_heartbeat.fid ());

    if (_verbose >= D_LOW) {
      warn << "heartbeat file: " << _heartbeat.fn () << "\n";
      warn << "sentinel file: " << _sentinel.fn () << "\n";
    }

    DIR *d = opendir (_dir.cstr ());
    bool rc = true;
    if (!d) {
      warn ("Cannot open root directory %s: %m\n", _dir.cstr ());
      rc = false;
    } else 
      closedir (d);
    return rc;
  }

  // doesn't return
  void run ();

private:
  void mark_files ();
  bool explore_dir (const str &d);
  bool sweep_files ();

  str _dir;
  file1_t _sentinel;
  file1_t _heartbeat;
  filetab_t _tab;
  bhash<fid_t> _skiptab;
  int _check_interval;
  int _verbose;
};

static treestat_t *g_tree;

static void usage ()
{
  warn << "usage: " << progname << " [-dvF] [-h <heartbeat file>] "
       << "[-s <sentinel file>]\n" 
       << "     [-f <configfile>] [-i <interval>] [-m <mode>] [<directory>]\n"
       << "\n"
       << "  default values:\n"
       << "    -d         false   (debug mode off by default)\n"
       << "    -vvvv      0       (verbosity - can specify >1 time)\n"
       << "    -i         1       (check interval = 1 second)\n"
       << "    -m         0644    (mode for .file writes)\n"
       << "    -s         .treestat_sentinel\n"
       << "    -h         .treestat_heartbeat\n"
       << "    -f         /usr/local/etc/okws/pub_config\n"
       << "    -F         false   (skip_config off by default)\n"
       << "    directory  .\n";
  exit (1);
}

tamed static void
main_T (int argc, char **argv)
{
  tvars {
    int ch;
    int i (1);
    str s, h, d;
    bool dbg (false);
    int vlev (0);
    int mode (-1);
    str configfile;
    bool skip_config (false);
    str u, g;
    bool rc;
    ptr<pub2::configger_t> cfg (pub2::configger_t::alloc ());
  }

  while ((ch = getopt (argc, argv, "i:s:h:dvm:f:Fu:g:")) != -1) {
    switch (ch) {
    case 'i':
      if (!convertint (optarg, &i)) {
	usage ();
      }
      break;
    case 'F':
      skip_config = true;
      break;
    case 'f':
      configfile = optarg;
      break;
    case 's':
      s = optarg;
      break;
    case 'h':
      h = optarg;
      break;
    case 'd':
      dbg = true;
      break;
    case 'v':
      vlev ++;
      break;
    case 'u':
      u = optarg;
      break;
    case 'g':
      g = optarg;
      break;
    case 'm':
      if (!convertint (optarg, &mode))
	usage ();
      break;
    default:
      usage ();
    }
  }
  argv += optind;
  argc -= optind;

  if (argc == 1) {
    d = argv[0];
  } else if (argc > 1) {
    usage ();
  }

  if (configfile && skip_config) {
    warn << "Cannot specify both -F and -f!\n";
    usage ();
  }
  if (!skip_config) {
    if (!configfile)
      configfile = okws_etcfile (ok_pub_config);
    if (configfile) {
      if (vlev >= D_LOW) {
	warn << "Using config file: " << configfile << "\n";
      }
      twait { cfg->run_cfg (configfile, mkevent (rc)); }
      if (!rc) {
	warn << "Could not parse: " << configfile << "\n"
	     << "  use -F to run without a config file\n";
      } else {
	if (!d) d = cfg->cfg ("JailDir");
	if (!s) s = cfg->cfg ("TreeStatSentinel");
	if (!h) h = cfg->cfg ("TreeStatHeartBeat");
	if (!u) u = cfg->cfg ("RunAsUser", true);
	if (!g) g = cfg->cfg ("RunAsGroup", true);

	str ts;
	int ti (0);
	if (mode < 0 && (ts = cfg->cfg ("TreeStatMode")) &&
	    convertint (ts, &ti))
	  mode = ti;
	  
      }
    }
  }

  if (!d || !d.len ()) d = ".";
  if (!s || !s.len ()) s = ok_pub2_treestat_sentinel;
  if (!h || !h.len ()) h = ok_pub2_treestat_heartbeat;
  if (mode < 0) mode = 0644;

  // Calls fatal() on failure
  cfg->setprivs (d, u, g);

  if (vlev >= D_LOW) {
    warn << "Running over directory: " << d << "\n";
  }

  g_tree = New treestat_t (d, 
			   cfg->jail2real (s), 
			   cfg->jail2real (h), 
			   i, 
			   vlev, 
			   mode);

  cfg = NULL;

  if (!g_tree->init ())
    exit (1);

  if (!dbg) {
    daemonize ();
  }
  warn ("%s starting up, pid %d\n", progname.cstr (), int (getpid ()));

  g_tree->run ();
}

void
treestat_t::mark_files ()
{
  for (file2_t *fp = _tab.first (); fp; fp = _tab.next (fp)) {
    fp->_mark_bit = true;
    fp->_visited = false;
  }
}

bool
treestat_t::sweep_files ()
{
  bool rc = false;
  file2_t *np;
  for (file2_t *fp = _tab.first (); fp; fp = np) {
    np = _tab.next (fp);
    if (fp->_mark_bit) {

      if (_verbose >= D_LOW)
	warn << "File removed: " << fp->_fn << "\n";

      rc = true;
      _tab.remove (fp);
      delete (fp);
    }
  }
  return rc;
}

bool
treestat_t::explore_dir (const str &d)
{
  DIR *dir = opendir (d.cstr ());
  struct dirent *de;
  bool rc = false;

  if (!dir) {
    if (_verbose >= D_LOW)
      warn ("Failed to open dir %s: %m\n", d.cstr ());
    return false;
  }
  if (_verbose >= D_HI) 
    warn ("Exploring directory %s\n", d.cstr ());

  while ((de = readdir (dir))) {
    str fn = apply_container_dir (d, de->d_name);
    struct stat sb;
    if (stat (fn.cstr (), &sb) >= 0) {
      const mode_t &m = sb.st_mode;
      fid_t fid (sb);
      file2_t *f = _tab[fid];
      bool visited = false;    // Don't go around in a circle

      if ((S_ISDIR (m) || S_ISREG (m) || S_ISLNK (m))) {
	if (!_skiptab[fid] && de->d_name[0] != '.') {
	  if (!f) {
	    if (_verbose >= D_MED)
	      warn << "New file found: " << fn << "\n";
	    
	    _tab.insert (New file2_t (fn, fid, sb.st_ctime));
	    rc = true;
	  } else {
	    f->_mark_bit = false;
	    if (sb.st_ctime > f->_ctime) {
	      if (_verbose >= D_LOW)
		warn << "file updated: " << fn << "\n";
	      
	      rc = true;
	      f->_ctime = sb.st_ctime;
	    }
	    visited = f->_visited;
	    f->_visited = true;
	  }

	  if (S_ISDIR (sb.st_mode) && !visited && explore_dir (fn))
	    rc = true;

	} else {
	  if (_verbose >= D_HI) 
	    warn ("Skipped file: %s\n", fn.cstr ());
	}
      } else {
	if (_verbose >= D_LOW)
	  warn ("Failed to open file %s: %m\n", fn.cstr ());
      }
    }
  }
  closedir (dir);
  return rc;
}

void
treestat_t::run ()
{
  bool dirty;
  while (1) {
    dirty = false;

    if (_verbose >= D_HI) 
      warn << "woke up...\n";

    mark_files ();
    if (explore_dir (_dir)) dirty = true;
    if (sweep_files ())     dirty = true;

    if (dirty) {
      if (_verbose >= D_LOW) {
	warn << "retouching sentinel due to dirty bit\n";
      }
      _sentinel.touch ();
    }
    _heartbeat.touch ();

    if (_verbose >= D_HI) 
      warn << "back to sleep...\n";
    
    sleep (_check_interval);
  }
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main_T (argc, argv);
  amain ();
}
