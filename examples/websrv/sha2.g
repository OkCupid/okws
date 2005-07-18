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
#include "sha_prot.h"

// see comments in sha.g; this file is almost exactly the same,
// except it using the run-time templating system

class oksrvc_sha_t : public oksrvc_t {
public:
  oksrvc_sha_t (int argc, char *argv[]) : oksrvc_t (argc, argv) 
  {
    // adds a capability; according to sha_prot.x, BAR_TOK
    // enables sha2.g to access SHA_QUERY2, but not
    // SHA_QUERY.
    authtoks.push_back (BAR_TOK);
    shadb = add_db ("rael.lcs.mit.edu", SHAD_PORT, sha_prog_1,
		    SHA_PROG_SHA_VERS_TXA_LOGIN);
  }
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
  dbcon_t *shadb;
};

class okclnt_sha_t : public okclnt_t {
public:
  okclnt_sha_t (ptr<ahttpcon> x, oksrvc_sha_t *o) 
      : okclnt_t (x, o), ok_sha (o) {}
  ~okclnt_sha_t () {}
  void process ()
  {
    if (cgi.blookup ("z")) {
      str x = cgi["x"];
      if (!x)
	error_page ("Error: no input given!");
      else {
	ptr<sha_query_res_t> res = New refcounted<sha_query_res_t> ();
	ok_sha->shadb->call (x[0] == 'a' ? SHA_QUERY2 : SHA_QUERY, &x, res,
			     wrap (this, &okclnt_sha_t::cb, res));
      }
    } else {
      output_page ();
    }
  }
  void cb (ptr<sha_query_res_t> res, clnt_stat err)
  {
    if (err)
      error_page (strbuf () << err);
    else if (res->status == ADB_NOT_FOUND)
      error_page ("Word not found in dictionary!");
    else if (res->status != ADB_OK)
      error_page ("Database errorr encountered!");
    else
      success_page (*res->res);
  }

  // the pub filter makes small rearrangements to this function
  // call; in particular, it turns the Perl-Like Associative Array
  // in the fourth argument of the "include" function into a well-formed
  // C++ datatype.  The output will be available in the build directly,
  // but also in ~max/oksamples/aux/sha2.C for demonstration purposes.
  void output_page (str s = NULL)
  {
    s = s ? str (strbuf ("<font face=helvetica color=blue size=+1>") 
      << s << "</font><br><hr>") :  str ("");
    /*o
      include (out, "/sha.html", { S => @{s} });
    o*/
    output (out);
  }

  void error_page (str s) 
  { output_page (strbuf ("<font color=red>Error[") << cgi["x"] <<
		 "]: " << s << "</font>"); }
  void success_page (str s)
  { output_page (strbuf ("SHA-1(\"") << cgi["x"] << "\"): " << s); }

  oksrvc_sha_t *ok_sha;
};

okclnt_t *
oksrvc_sha_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_sha_t (x, this); 
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_sha_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
