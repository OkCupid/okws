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

class oksrvc_bomb_t : public oksrvc_t {
public:
  oksrvc_bomb_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
};

class okclnt_bomb_t : public okclnt_t {
public:
  okclnt_bomb_t (ptr<ahttpcon> x, oksrvc_bomb_t *o) 
      : okclnt_t (x, o), ok_bomb (o) {}
  ~okclnt_bomb_t () {}


  // will become unrepsonsive and will never return to the core
  // select loop; using this to test per service quotas; one misbehaved
  // service should not bring down the whole site.
  //
  void process ()
  {
    while (1) {
      sleep (1);
    }
  }

  oksrvc_bomb_t *ok_bomb;
};

okclnt_t *
oksrvc_bomb_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_bomb_t (x, this); 
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_bomb_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
