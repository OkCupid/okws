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
#include "json_rpc.h"

//-----------------------------------------------------------------------

class oksrvc_encoder_t : public oksrvc_t {
public:
  oksrvc_encoder_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
protected:
};

//-----------------------------------------------------------------------

class okclnt_encoder_t : public okclnt2_t {
public:
  okclnt_encoder_t (ptr<ahttpcon> x, oksrvc_encoder_t *o)
    : okclnt2_t (x, o), ok_encoder (o) {}
  ~okclnt_encoder_t () {}

  void process (proc_ev_t ev) { process_T (ev); }
  void process_T (proc_ev_t ev, CLOSURE); 
protected:
  void do_encode (str i, str typ);
  void do_decode (str s, str typ);
  void set_output (bool ok, str dat);
  pub3::obj_dict_t m_obj;
  oksrvc_encoder_t *ok_encoder;
};

//-----------------------------------------------------------------------

void
okclnt_encoder_t::set_output (bool b, str m)
{
  m_obj ("ok") = b;
  m_obj ("data") = m;
}

//-----------------------------------------------------------------------

void
okclnt_encoder_t::do_decode (str s, str typ)
{
  str out;
  str raw;
  ptr<pub3::expr_t> x;
  
  if (!(raw = dearmor64 (s))) {
    set_output (false, "could not dearmor input");
  } else if (!(x = xdropq2json (typ, raw))) {
    set_output (false, "could xdropq2json failed");
  } else {
    m_obj("status") = true;
    m_obj("data") = x;
  }
}

//-----------------------------------------------------------------------

void
okclnt_encoder_t::do_encode (str s, str typ)
{
  str out;
  ptr<pub3::expr_t> x;
  str raw;

  if (!(raw = dearmor64 (s))) {
    set_output (false, "could not dearmor64 input");
  } else if (!(x = pub3::json_parser_t::parse (raw))) {
    set_output (false, "failed to parse input as JSON");
  } else if (!json2xdropq (typ, &out, x)) {
    set_output (false, "json2xdropq failed");
  } else {
    set_output (true, armor64 (out));
  }
}

//-----------------------------------------------------------------------

tamed void 
okclnt_encoder_t::process_T (okclnt2_t::proc_ev_t ev)
{
  tvars {
    str in;
    str typ;
    str tmp;
  }

  if (!cgi.lookup ("type", &typ)) {
    set_output (false, "no type sepcified");
  } else if (cgi.lookup ("encode", &in)) {
    do_encode (in, typ);
  } else if (cgi.lookup ("decode", &in)) {
    do_decode (in, typ);
  } else {
    set_output (false, "usage: specify encode=X or decode=X");
  }
  tmp = m_obj.to_str ();
  out << tmp;
  twait { output (out, mkevent ()); }
  ev->trigger (true, HTTP_OK);
}

//-----------------------------------------------------------------------

oksrvc_t::newclnt_t *
oksrvc_encoder_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_encoder_t (x, this);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_encoder_t (argc, argv);
  oksrvc->launch ();
  json_XDR_dispatch_t::enable ();
  amain ();
}

//-----------------------------------------------------------------------

