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
#include "tame.h"
#include "rxx.h"
#include "mimetypes.h"

class oksrvc_static_t : public oksrvc_t {
public:
  oksrvc_static_t (int argc, char *argv[]) 
    : oksrvc_t (argc, argv), _mtm (NULL) {}

  okclnt_base_t *make_newclnt (ptr<ahttpcon> x);
  void post_launch_pub2_T (cbb cb, CLOSURE);
  void post_launch_pub2 (cbb cb) { post_launch_pub2_T (cb); }
  const mime_type_map_t *mime_type_map () const { return _mtm; }

private:
  mime_type_map_t *_mtm;
};

class okclnt_static_t : public okclnt_t {
public:
  okclnt_static_t (ptr<ahttpcon> x, oksrvc_static_t *o)
    : okclnt_t (x, o), ok_static (o) {}
  ~okclnt_static_t () {}

  void process () { process_T (); }
  void process_T (CLOSURE); 

protected:
  void error (const str &desc);
  str ofn;
  bool rc;
  aarr_t aarr;
  oksrvc_static_t *ok_static;
};

#define MIME_TYPES_FILE "conf/mime.types"
#define FNF_FILE "/fnf.html"

TAMED void 
oksrvc_static_t::post_launch_pub2_T (cbb cb)
{
  VARS {
    bool res1, res2, res3;
  }
  BLOCK { 
    oksrvc_t::post_launch_pub2 (@(res1));
    pub2 ()->run_cfg ("conf/intl.conf", @(res2));
    pub2 ()->run_cfg (MIME_TYPES_FILE, @(res3));
  }
  if (res3) 
    _mtm = New mime_type_map_t ((*(pub2 ()))["MimeTypes"]);

  SIGNAL (cb, res1 && res2);
}

void
okclnt_static_t::error (const str &s)
{
  aarr.add ("target", s);
  ofn = FNF_FILE;
  rc = false;
}

TAMED void 
okclnt_static_t::process_T ()
{
  VARS {
    str in;
    u_int opts (P_IINFO|P_VERBOSE);
    mime_type_map_t *mtm;
    str sffx;
    str mime_type;
  }
  rc = true;
  static rxx x ("^(:\\d+)?/static(/.+)$");
  in = hdr.get_target ();

  if (!(mtm = ok_static->mime_type_map ())) {
    error (MIME_TYPES_FILE);

  } else if (x.match (in)) {

    ofn = x[2];
    assert (ofn);

    if ((mime_type = mtm->lookup (ofn, &sffx))) {

      if (cicmp (sffx, "html")) {
	cgi.load_aarr (&aarr);
      } else {
	opts |= P_NOPARSE;
      }
      set_content_type (mime_type);
      
      BLOCK { pub2 ()->run (&out, ofn, @(rc), &aarr, opts); }

      if (!rc) 
	error (ofn);
      
    } else {
      strbuf b;
      b << "&lt; <i>no mime-type found for suffix '"
	<< sffx << "'</i> &gt;";
      error (b);
    }

  } else {
    error ("&lt; <i>no file specified</i> &gt;");
  }

  if (!rc) {
    BLOCK { pub2 ()->run (&out, ofn, @(rc), &aarr, opts|P_VISERR); }
  }

  output (out);
}

okclnt_base_t *
oksrvc_static_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_static_t (x, this);
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_static_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
