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

#include "rpcc.h"
#include "ok.h"
#include "okcgi.h"
#include "pub.h"
#include <unistd.h>

#include "pt1_prot.h"
#include "amysql.h"

class oksrvc_pt1_t : public oksrvc_t {
public:
  oksrvc_pt1_t (int argc, char *argv[]) : oksrvc_t (argc, argv) 
  {
    db = add_db ("10.1.1.20", PT1_PORT, pt1_prog_1);
  }
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
  dbcon_t *db;
};

class okclnt_pt1_t : public okclnt_t {
public:
  okclnt_pt1_t (ptr<ahttpcon> x, oksrvc_pt1_t *o) 
      : okclnt_t (x, o), ok_pt1 (o) {}
  ~okclnt_pt1_t () {}
  dbcon_t *db () const { return ok_pt1->db; }
  void process ()
  {
    if(!cgi.lookup ("id", &id)) {
       out << "Input (CGI var 'id') needs to be an integer \n";
       output (out);
    } else {
       db()->call(PT1_QUERY, &id, &res, wrap(this, &okclnt_pt1_t::qry_cb));
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
      /*o 
	include (pub, out, "/pt2.html", 
	{ sha => @{res.out->sha1}, rid => @{res.out->id}, id => @{id}} ); 
	o*/
    }
    output (out);
  }
  int id; // make id global for qry_cb()
  oksrvc_pt1_t *ok_pt1;
  pt1_query_res_t res;
};

okclnt_t *
oksrvc_pt1_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_pt1_t (x, this); 
}

int
main (int argc, char *argv[])
{
  //hlpr_max_qlen = 10000;
  hlpr_max_calls = 2000;
  oksrvc_t *oksrvc = New oksrvc_pt1_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
