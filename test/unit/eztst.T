// -*-c++-*-
/* $Id: eztst.g 1007 2005-09-11 21:45:33Z max $ */

/*
 *
 * Copyright (C) 2003-4 by Maxwell Krohn (max@okcupid.com)
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

#include "ok.h"
#include "okcgi.h"
#include "pub.h"
#include <unistd.h>

#include "amysql.h"
#include "ezdb.h"

//-----------------------------------------------------------------------

class tester_t {
public:
  tester_t () : _host ("localhost"), _port (EZDBD_PORT) {}
  void run (evi_t ev, CLOSURE);
  void init (evi_t ev, CLOSURE);
  bool config (int argc, char **argv);
private:
  ptr<ezdb::cli_t> _ezdb;
  str _host;
  int _port;
};

//-----------------------------------------------------------------------

static void 
usage ()
{
  warnx << "usage: " << progname << " [-h <host>] [-p <port>]\n";
}

//-----------------------------------------------------------------------

bool
tester_t::config (int argc, char *argv[])
{
  int ch;
  bool ret = true;
  while ((ch = getopt (argc, argv, "h:p:")) != -1) {
    switch (ch) {
    case 'h':
      _host = optarg;
      break;
    case 'p':
      if (!convertint (optarg, &_port)) {
	warn << "Cannot convert port='" << optarg << "' to int\n";
	ret = false;
      }
      break;
    default:
      ret = false;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

tamed void
tester_t::init (evi_t ev)
{
  tvars {
    int rc (0);
    bool ok;
  }
  _ezdb = New refcounted<ezdb::cli_t> (_host, _port);
  twait { _ezdb->connect (mkevent (ok)); }
  if (!ok) {
    warn << "Ezdb failed to connect!\n";
    rc = -1;
  }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed void
tester_t::run (evi_t ev)
{
  tvars {
    ptr<ezdb::sth_t> sth;
    time_t now (sfs_get_timenow ());
    okdate_t d (now), d1, d2;
    str s;
    int pid;
    adb_status_t c;
    str err;
    size_t i (0);
    size_t v;
    int64_t i64;
    vec<str> ids;
    bool go;
    size_t id;
  }

  pid = getpid ();
  sth = _ezdb->EZ_PREPARE ("INSERT INTO tst2 (s, d, i, d2) VALUES(?,?,?,?)");
  for (i = 0; i < 3; i++) {
    s = strbuf () << "random insert " << pid << "@" << now << ":" << i;
    ids.push_back (s);
    v = pid + now + i;
    twait { sth->execute (mkevent (c, err), s, d, v, d); }
    if (c != ADB_OK) {
      warn << "MySQL ez error: " << err << "\n";
    } else {
      warn << "Insert(" << s << ", " << v << ") -> " << 
	sth->insert_id () << "\n";
    }
  }

  sth = _ezdb->EZ_PREPARE ("SELECT id, d, i, d2 FROM tst2 WHERE s=?");
  for (i = 0; i < ids.size (); i++) {
    twait { sth->execute (mkevent (c, err), ids[i]); }
    go = true;
    while (go) {
      go = false;
      twait { sth->fetch (mkevent (c, err), &id, &d1, &i64, &d2); }
      if (c == ADB_OK) {
	go = true;
	str s1 = d1.to_str ();
	str s2 = d2.to_str ();
	warn << "Select(" << ids[i] << ") -> (" << id << "," << s1 
	     << "," << s2 << ")\n";
      } else if (c != ADB_NOT_FOUND) {
	warn << "MySQL ez error: " << err << "\n";
      }
    }
  }

  sth = _ezdb->EZ_PREPARE ("SELECT id, s, d, i, d2 FROM tst2");
  twait { sth->execute (mkevent (c, err)); }
  if (c != ADB_OK) {
    warn << "MySQL ez error: " << err << "\n";
  } else {
    while (c == ADB_OK) {
      twait { sth->fetch (mkevent (c, err), &id, &s, &d1, &i64, &d2); }
      if (c == ADB_OK) {
	str s1 = d1.to_str ();
	str s2 = d2.to_str ();
	warn << "Select(ALL) -> (" << id << "," << s << "," << s1  
	     << "," << i64 << "," << s2 << ")\n";
      } else if (c != ADB_NOT_FOUND) {
	warn << "MySQL ez error: " << err << "\n";
      }
    }
  }
  twait { sth->finish (mkevent (c, err)); }
  ev->trigger (0);
}

//-----------------------------------------------------------------------

tamed static void
main_T (int argc, char **argv)
{
  tvars {
    tester_t tester;
    int rc (0);
  }

  if (!tester.config (argc, argv)) { 
    rc = -2; 
    usage ();
  }

  if (rc == 0) { twait { tester.init (mkevent (rc)); } }
  if (rc == 0) { twait { tester.run (mkevent (rc)); } }

  exit (rc);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main_T (argc, argv);
  amain ();
}

//-----------------------------------------------------------------------
