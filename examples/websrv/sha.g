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

//
// oksrvc_sha_t: implements the "sha" service. One global instance
//   of this object will be created when the server boots up. When
//   a new client request comes in, this class will call 
//   make_newclnt (), which will allocate a new object of
//   type okclnt_sha_t:
//
// okclnt_sha_t: every external connection to this service will
//   correspond to exaclty one instance of an okclnt_sha_t object.
//   this object will call routines that external client's
//   HTTP headers, will generate a result, and return it accordingly.
//   The object is destroyed immediately after the request is sent
//   and the connection is closed.
//
// Note that unless otherwise stated, this file represents
//   boiler-plate code; the web service developer typically starts
//   developing at the process () function, which will process an
//   external user request.

class oksrvc_sha_t : public oksrvc_t {
public:
  oksrvc_sha_t (int argc, char *argv[]) : oksrvc_t (argc, argv) 
  {
    // Non-boiler-plate: connect to a particular db proxy
    shadb = add_db ("rael.lcs.mit.edu", SHAD_PORT, sha_prog_1);
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

  // non-boiler plate: process a user request to the SHA query service.
  void process ()
  {
    if (cgi.blookup ("z")) {
      str x = cgi["x"];
      if (!x)
	error_page ("Error: no input given!");
      else {
	ptr<sha_query_res_t> res = New refcounted<sha_query_res_t> ();
	ok_sha->shadb->call (SHA_QUERY, &x, res,
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

  // Note that this file is processed by the pub filter; this is a simple
  // "heredoc" implementation for C++.  Go to ~max/oksamples/aux/sha.C to
  // see the corresponding .C file, which is output from the filter.
  // The output file is more important found in the build directory.
  void output_page (str s = NULL)
  {
    /*o
      print (out) <<EOF;
<html>
 <head>
  <title>SHA1 Dictionary Querier</title>
 </head>
<body>
EOF
o*/

    if (s) 
      /*o
	print (out) <<EOF;
<font face=helvetica color=blue size=+1>@{s}</font>
<br><hr>
EOF
o*/

    /*o
      print (out) <<EOF;
<b>Ask Me For the SHA of your Favorite Word</b>
<form action="/sha">
Ask Away: <input type=text name=x> &nbsp;
<input type=submit name=submit value=submit>
<input type=hidden name=z value=1>
</form>
</body>
</html>
EOF
o*/
    output (out);
  }

  void error_page (str s) 
  { output_page (strbuf ("<font color=red>Error: ") << s << "</font>"); }

  void success_page (str s)
  { output_page (strbuf ("SHA-1(\"") << cgi["x"] << "\"): " << s); }
  
  // end non-boiler plate code;

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
