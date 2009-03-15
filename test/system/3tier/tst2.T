// -*-c++-*-
/* $Id: tst2.g 1007 2005-09-11 21:45:33Z max $ */

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

#include "tst2_prot.h"
#include "amysql.h"

//-----------------------------------------------------------------------

class oksrvc_tst2_t : public oksrvc_t {
public:
  oksrvc_tst2_t (int argc, char *argv[]) : oksrvc_t (argc, argv) 
  {
    const char *c;
    str dbs ("127.0.0.1");
    if ((c = getenv ("DB_HOST")))
      dbs = c;
    _db = add_db (dbs, TST2_PORT, tst2_prog_1);
  }
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
  dbcon_t *db() { return _db; }
private:
  dbcon_t *_db;
};

//-----------------------------------------------------------------------

class okclnt_tst2_t : public okclnt2_t {
public:
  okclnt_tst2_t (ptr<ahttpcon> x, oksrvc_tst2_t *o) 
    : okclnt2_t (x, o), _svc (o) {}
  ~okclnt_tst2_t () {}
  dbcon_t *db () const { return _svc->db (); }
  void process (proc_ev_t ev) { process_T (ev); }
private:
  void process_T (proc_ev_t ev, CLOSURE);
  oksrvc_tst2_t *_svc;
};

//-----------------------------------------------------------------------

tamed void
okclnt_tst2_t::process_T (okclnt2_t::proc_ev_t ev)
{
  tvars {
    tst2_get_arg_t garg;
    tst2_get_res_t gres;
    tst2_put_arg_t arg;
    adb_status_t res;
    clnt_stat err;
    okdate_t before (okwstime ());
    okdate_t after, after2;
    str key;
  }

  {
    strbuf b;
    b << getpid () << "." << okwstime ();
    key = b;
  }

  arg.key = key;
  before.to_xdr (&arg.data.d);
  arg.data.i = getpid ();
  arg.data.pk = 0;
  before.to_xdr (&arg.data.d2);

  out << "<html><head><title>Test2</title></head></html>\n";
  out << "<body>\n";

  twait { db ()->call (TST2_PUT, &arg, &res, mkevent (err)); }
  if (err) { 
    out << "<font color=red>Put error: " << err << "</font>\n";
  } else {
    garg = key;
    twait { db ()->call (TST2_GET, &garg, &gres, mkevent (err)); }
    if (err) {
      out << "<font color=red>Get error: " << err << "</font>\n";
    } else if (gres.status != ADB_OK) {
      out << "<font color=red>Get error: " << int(gres.status) << "\n";
    } else {
      after.set (gres.dat->d);
      after2.set (gres.dat->d2);
      str s1 = before.to_str ();
      str s2 = after.to_str ();
      str s3 = after2.to_str ();
      out << "<b>before:</b> " << s1 << "\n<br>"
	  << "<b>after1:</b>  " << s2 << "\n<br>"
	  << "<b>after2:</b>  " << s3 << "\n<br>";
    }
  }
  out << "</body>\n"
      << "</html>\n";
  twait { output (out, mkevent ()); }
  ev->trigger (true, HTTP_OK);
}


//-----------------------------------------------------------------------

oksrvc_t::newclnt_t *
oksrvc_tst2_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_tst2_t (x, this); 
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_tst2_t (argc, argv);
  oksrvc->launch ();
  amain ();
}

//-----------------------------------------------------------------------
