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
#include "pt1_prot.h"

#define DB "10.1.1.20"

class oksrvc_pt5_t : public oksrvc_t {
public:
  oksrvc_pt5_t (int argc, char *argv[]) : oksrvc_t (argc, argv)  
  {
    db = add_db (DB, PT1_PORT, pt1_prog_1);
  }
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
  dbcon_t *db;
};

class okclnt_pt5_t : public okclnt_t {
public:
  okclnt_pt5_t (ptr<ahttpcon> x, oksrvc_pt5_t *o) 
      : okclnt_t (x, o), ok_pt5 (o) {}
  ~okclnt_pt5_t () {}
  inline dbcon_t *db () const { return ok_pt5->db; }
  void process ()
  {
    db ()->call (PT1_TIMES_TAB, NULL, &res, 
		 wrap (this, &okclnt_pt5_t::tt_cb));
  }

  void tt_cb (clnt_stat err)
  {
    if (err) {
      warn << "RPC Error : " << err << "\n";
      out << "Connection to DB failed\n";
    } else if (res.status == ADB_EXECUTE_ERROR) {
      warn << "Error querying database\n";
      out << "Error querying database\n";
    } else {
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
      size_t sz = 7 * TT_ROWS * TT_COLS;
      mstr m (sz);
      char *base = m.cstr ();
      size_t n = 0;
      size_t nrows  = res.tab->size ();
      for (u_int i = 0; i < nrows; i++) {
	for (u_int j = 0; j < (*res.tab)[i].size (); j++) {
	  if (n >= sz) break;
	  n += snprintf (base + n, sz - n, " %d\n", (*res.tab)[i][j]);
	}
      }
      m.setlen (min<size_t> (sz, n));
      out << str (m) << "</table>\n</body>\n</html>\n";

    }
    output (out);
  }
  int id; // make id global for qry_cb()
  oksrvc_pt5_t *ok_pt5;
  pt1_times_tab_res_t res;
};

okclnt_t *
oksrvc_pt5_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_pt5_t (x, this); 
}

int
main (int argc, char *argv[])
{
  //hlpr_max_qlen = 10000;
  hlpr_max_calls = 2000;
  oksrvc_t *oksrvc = New oksrvc_pt5_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
