// -*-c++-*-
/* $Id$ */

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

#include "pt1_prot.h"
#include "amysql.h"

#define DB "127.0.0.1"
#define THOUSAND 1000
#define MILLION THOUSAND * THOUSAND

/**
 * A service class for Max Krohn's master's thesis.
 * Trying to test basic performance parameters by varying 
 * whether there is a DB query or not, the size of the 
 * the response, whether or not the junk is dynamically
 * generated or not, and whether or not the publishing
 * system is going to be used.
 */
class oksrvc_mt1_t : public oksrvc_t {
public:

  /**
   * Connect to database on startup; the database is assumed to 
   * be on localhost, but can be set with the DB_HOST environment
   * variable.
   */
  oksrvc_mt1_t (int argc, char *argv[]) 
    : oksrvc_t (argc, argv), junk_sz (0)
  {
    const char *c;
    str dbs (DB);
    if ((c = getenv ("DB_HOST")))
      dbs = c;
    db = add_db (dbs, PT1_PORT, pt1_prog_1);
  }

  // the rest of this stuff is just boiler plate
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
  dbcon_t *db;
  
  str junk; // junk stored on the client side
  size_t junk_sz;
};


class okclnt_mt1_t : public okclnt_t {
public:
  okclnt_mt1_t (ptr<ahttpcon> x, oksrvc_mt1_t *o) 
    : okclnt_t (x, o), ok_mt1 (o), sz (0), reuse (false),
      dbcall (false), nopub (false) {}
  ~okclnt_mt1_t () {}
  dbcon_t *db () const { return ok_mt1->db; }

  /**
   * columns in the 'junk table'
   */
  enum { COLS = 10 };

  /**
   * Write junk to either a zbuf, or a strbuf
   */
  template<class T>
  void junk_to_sink (T &b)
  {
    if (!sz)
      return;

    b << "<table>\n"
      " <tr>\n";
    for (u_int i = 0; i < sz; i++) {
      if (i != 0 && i % COLS == 0) {
	b << " </tr>\n <tr>\n";
      }
      b << "<td>" << i << "</td>\n";
    }
    b << " </tr>\n"
      << "</table>\n";
  }

  /**
   * Generate the junk
   */
  str generate_junk ()
  {
    if (!sz)
      return "";
    strbuf b;
    junk_to_sink (b);
    return b;
  }

  /**
   * Either generate the junk or get it from the cache if it's
   * pregenerated
   */
  str get_junk ()
  {
    if (!reuse)
      return generate_junk ();

    if (!ok_mt1->junk || sz != ok_mt1->junk_sz) {
      ok_mt1->junk = generate_junk ();
      ok_mt1->junk_sz = sz;
    }
    return ok_mt1->junk;
  }

  /**
   * Dump junk to the output sink!
   */
  void dump_junk ()
  {
    if (sz) {
      if (reuse) {
	out << get_junk ();
      } else {
	junk_to_sink (out);
      }
    }
  }

  /**
   * Do all of the output of this service, thinking about different
   * output modes mentioned above
   */
  void do_output (bool call_finish)
  {
    if (nopub) {
      /*o
	print (out) <<EOF;
<html>
 <head>
  <title>MT1 Resxlt</title>
 </head>
<body>
EOF
       o*/

      dump_junk ();

      if (dbcall) {
	/*o
	  print (out) <<EOF;
DBRES @{id} @{res.out->id} @{res.out->sha1}
EOF
         o*/
      } else {
	out << "RES\n";
      }
      /*o
	print (out) <<EOF;
</body>
</html>
EOF
      o*/

    } else {
      if (dbcall) {
	/*o 
	  include (pub, out, "/mt1db.html", 
	  { sha => @{res.out->sha1}, rid => @{res.out->id}, id => @{id},
	    junk => @{get_junk ()} }); 
	  o*/
      } else {
	/*o 
	  include (pub, out, "/mt1.html", { junk => @{get_junk ()} }); 
	  o*/
      }
    }
    if (call_finish)
      output (out);
  }

  void process ()
  {
    // the size of the junk
    cgi.lookup ("sz", &sz);

    // if we're filling up with junk, whether or not to cache the
    // junk so that we don't need to keep allocating it
    reuse = cgi.blookup ("reuse");

    // whether or not to use the Publishing system
    nopub = cgi.blookup ("nopub");

    u_int tmp;

    if (cgi.lookup ("id", &id)) {
      dbcall = true;
      db ()->call (PT1_QUERY, &id, &res, wrap (this, &okclnt_mt1_t::qry_cb));
    } else if (cgi.lookup ("wt", &tmp)) {
      delaycb (tmp / THOUSAND, (tmp % THOUSAND) / MILLION, 
	       wrap (this, &okclnt_mt1_t::do_output, true));
    } else {
      do_output (true);
    }
  }

  void qry_cb (clnt_stat err) 
  {
    if(err) {
      warn << "RPC Error : " << err << "\n";
      out << id << "Connection to DB failed \n";
    } else if (res.status == ADB_EXECUTE_ERROR) {
      out << id << "Error querying database \n";
    } else {
      do_output (false);
    }
    output (out);
  }

  int id; // make id global for qry_cb()
  oksrvc_mt1_t *ok_mt1;
  pt1_query_res_t res;
  size_t sz;
  bool reuse, dbcall, nopub;
};


okclnt_t *
oksrvc_mt1_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_mt1_t (x, this); 
}

int
main (int argc, char *argv[])
{
  //hlpr_max_qlen = 10000;
  hlpr_max_calls = 2000;
  oksrvc_t *oksrvc = New oksrvc_mt1_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
