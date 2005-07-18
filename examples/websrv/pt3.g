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

#include "amysql.h"

class oksrvc_pt3_t : public oksrvc_t {
public:
  oksrvc_pt3_t (int argc, char *argv[]) : oksrvc_t (argc, argv)  {}
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
  dbcon_t *db;
};

class okclnt_pt3_t : public okclnt_t {
public:
  okclnt_pt3_t (ptr<ahttpcon> x, oksrvc_pt3_t *o) 
      : okclnt_t (x, o), ok_pt3 (o) {}
  ~okclnt_pt3_t () {}
  dbcon_t *db () const { return ok_pt3->db; }
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
    for (int i = 1; i <= 25; i++) {
      for (int j = 1; j <= 25; j++) {
	out << " " << i *j << "\n";
      }
    }
    output (out);
  }
  int id; // make id global for qry_cb()
  oksrvc_pt3_t *ok_pt3;
};

okclnt_t *
oksrvc_pt3_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_pt3_t (x, this); 
}

int
main (int argc, char *argv[])
{
  //hlpr_max_qlen = 10000;
  hlpr_max_calls = 2000;
  oksrvc_t *oksrvc = New oksrvc_pt3_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
