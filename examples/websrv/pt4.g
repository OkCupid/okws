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
#include "cgi.h"
#include "pub.h"
#include <unistd.h>

#include "amysql.h"

class oksrvc_pt4_t : public oksrvc_t {
public:
  oksrvc_pt4_t (int argc, char *argv[]) : oksrvc_t (argc, argv)  {}
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
  dbcon_t *db;
};

class okclnt_pt4_t : public okclnt_t {
public:
  okclnt_pt4_t (ptr<ahttpcon> x, oksrvc_pt4_t *o) 
      : okclnt_t (x, o), ok_pt4 (o) {}
  ~okclnt_pt4_t () {}
  dbcon_t *db () const { return ok_pt4->db; }
  void process ()
  {
    /*o
	print (out) <<EOF;
<html>
 <head>
  <title>PT3 Test</title>
 <head>
<body>
 <table>
EOF
    o*/

    size_t sz = 7 * 25 * 25;
    mstr m (sz);
    char *bp = m.cstr ();
    int i,j;
    for (i = 1; i <= 60; i++) {
      for (j = 1; j <= 10; j++) {
	bp += sprintf (bp, " %d\n", i*j);
      }
    }
    m.setlen (bp - m.cstr ());
    out << str (m) << "</table>\n</body>\n</html>\n";

    output (out);
  }
  int id; // make id global for qry_cb()
  oksrvc_pt4_t *ok_pt4;
};

okclnt_t *
oksrvc_pt4_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_pt4_t (x, this); 
}

int
main (int argc, char *argv[])
{
  //hlpr_max_qlen = 10000;
  hlpr_max_calls = 2000;
  oksrvc_t *oksrvc = New oksrvc_pt4_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
