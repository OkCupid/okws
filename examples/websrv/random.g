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
#include "crypt.h"
#include "pt1_prot.h"
//#include "amysql.h"

#define DB "10.1.1.20"

class oksrvc_random_t : public oksrvc_t {
public:
  oksrvc_random_t (int argc, char *argv[]) : oksrvc_t (argc, argv)  
  {
    //db = add_db (DB, PT1_PORT, pt1_prog_1);
  }
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
  dbcon_t *db;
  vec<str> rand_strs;
};

class okclnt_random_t : public okclnt_t {
public:
  okclnt_random_t (ptr<ahttpcon> x, oksrvc_random_t *o) 
      : okclnt_t (x, o), ok_random (o) {}
  ~okclnt_random_t () {}
  inline dbcon_t *db () const { return ok_random->db; }
  enum { rand_str_len = 1000 } ;

  str genrand () 
  {
    mstr m (rand_str_len);
    rnd.getbytes (m.cstr (), rand_str_len);
    return armor32 (m);
  }

  void process ()
  {
    for (u_int i = 0; i < 19; i++) {
      str s;
      if (rnd.getword () % 3 == 0) {
	s = ok_random->rand_strs[rnd.getword()%ok_random->rand_strs.size ()];
      } else {
	s = genrand ();
	ok_random->rand_strs.push_back (s);
      }
      out << s;
    }
    output (out);
  }
  int id; // make id global for qry_cb()
  oksrvc_random_t *ok_random;
};

okclnt_t *
oksrvc_random_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_random_t (x, this); 
}

int
main (int argc, char *argv[])
{
  //hlpr_max_qlen = 10000;
  hlpr_max_calls = 2000;
  oksrvc_t *oksrvc = New oksrvc_random_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
