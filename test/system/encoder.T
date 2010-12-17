// -*-c++-*-
/* $Id: encoder.T 4994 2009-12-12 02:56:24Z max $ */

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
#include "tame.h"
#include "rxx.h"
#include "ok_adebug.h"
#include "test_const.h"
#include "okprot.h"
#include "pub3prot.h"

class oksrvc_encoder_t : public oksrvc_t {
public:
  oksrvc_encoder_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
protected:
};

class okclnt_encoder_t : public okclnt2_t {
public:
  okclnt_encoder_t (ptr<ahttpcon> x, oksrvc_encoder_t *o)
    : okclnt2_t (x, o), ok_encoder (o) {}
  ~okclnt_encoder_t () {}

  void process (proc_ev_t ev) { process_T (ev); }
  void process_T (proc_ev_t ev, CLOSURE); 

protected:
  oksrvc_encoder_t *ok_encoder;
};

tamed void 
okclnt_encoder_t::process_T (okclnt2_t::proc_ev_t ev)
{
  ev->trigger (true, HTTP_OK);
}

oksrvc_t::newclnt_t *
oksrvc_encoder_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_encoder_t (x, this);
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_encoder_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
