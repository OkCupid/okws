// -*-c++-*-
/* $Id: upload.T 3188 2008-02-05 00:51:21Z max $ */

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

class oksrvc_upload_t : public oksrvc_t {
public:
  oksrvc_upload_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
};

class okclnt_upload_t : public okclnt_t {
public:
  okclnt_upload_t (ptr<ahttpcon> x, oksrvc_upload_t *o)
    : okclnt_t (x, o), ok_upload (o) {}
  ~okclnt_upload_t () {}

  void process () { process_T (); }
  void process_T (CLOSURE); 

protected:
  oksrvc_upload_t *ok_upload;
};

tamed void 
okclnt_upload_t::process_T ()
{
  tvars {
    aarr_t aarr;
    cgi_files_t *f;
    const char *field ("upfile");
    const char *file ("/upload.html");
    bool rc;
    size_t sz(0);
  }

  if (cgi.flookup (field, &f) && f->size()) {
    cgi_file_t file = f->pop_back ();
    sz = file.dat.len ();
  } 
  aarr.add ("sz", sz);

  twait { pub2 ()->run (&out, file, mkevent (rc), &aarr); }
  output (out);
}

oksrvc_t::newclnt_t *
oksrvc_upload_t::make_newclnt (ptr<ahttpcon> x)
{
  okclnt_t *cli = New okclnt_upload_t (x, this);
  cli->enable_file_upload ();
  return cli;
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_upload_t (argc, argv);
  oksrvc->launch ();
  ok_reqsize_limit = 0x100000; // 1MB
  amain ();
}
