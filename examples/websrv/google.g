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
#include <stdio.h>
#include <unistd.h>

class oksrvc_google_t : public oksrvc_t {
public:
  oksrvc_google_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
};

class okclnt_google_t : public okclnt_t {
public:
  okclnt_google_t (ptr<ahttpcon> x, oksrvc_google_t *o) 
      : okclnt_t (x, o), ok_google (o) {}
  ~okclnt_google_t () {}
  void process ()
  {
    redirect ("http://www.google.com");
  }
  oksrvc_google_t *ok_google;
};

okclnt_t *
oksrvc_google_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_google_t (x, this); 
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_google_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
