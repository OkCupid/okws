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
#include "oklocale.h"
#include "okws_sfs.h"
#include "test_const.h"

#if !defined(OKWS_AT_VERSION) || !OKWS_AT_VERSION(1,2,0,0)
# error "This service requires OKWS version 1.2.0 or greater!"
#endif

class oksrvc_static_t : public oksrvc_t {
public:
  oksrvc_static_t (int argc, char *argv[]) 
    : oksrvc_t (argc, argv), _mtm (NULL) {}

  newclnt_t *make_newclnt (ptr<ahttpcon> x);
  void post_launch_pub2_T (cbb cb, CLOSURE);
  void post_launch_pub2 (cbb cb) { post_launch_pub2_T (cb); }

  const mime_type_map_t *mime_type_map () const { return _mtm; }
  ptr<const std_locale::localizer_factory_t> lfact () const { return _lfact; }

private:
  mime_type_map_t *_mtm;
  ptr<std_locale::localizer_factory_t> _lfact;
};

class okclnt_static_t : public okclnt2_t {
public:
  okclnt_static_t (ptr<ahttpcon> x, oksrvc_static_t *o)
    : okclnt2_t (x, o), ok_static (o) {}
  ~okclnt_static_t () {}

  void process (proc_ev_t ev) { process_T (ev); }
  void process_T (proc_ev_t ev, CLOSURE); 

protected:
  void error (const str &desc);
  str ofn;
  bool rc;
  aarr_t aarr;
  oksrvc_static_t *ok_static;
};


tamed void 
oksrvc_static_t::post_launch_pub2_T (cbb cb)
{
  tvars {
    bool res1, res2, res3;
  }
  twait { 
    oksrvc_t::post_launch_pub2 (mkevent (res1));
    pub2 ()->run_cfg (INTL_CONF_FILE,  mkevent (res2));
    pub2 ()->run_cfg (MIME_TYPES_FILE, mkevent (res3));
  }
  if (res3) {
    _mtm = New mime_type_map_t ((*(pub2 ()))["MimeTypes"]);
  }
  if (res2) {
    str fmt = (*(pub2 ()))["LocaleFormat"];
    if (fmt)
      _lfact = std_locale::localizer_factory_t::alloc (fmt);
  }

  TRIGGER (cb, res1 && res2);
}

void
okclnt_static_t::error (const str &s)
{
  aarr.add ("target", s);
  ofn = FNF_FILE;
  rc = false;
}


tamed void 
okclnt_static_t::process_T (okclnt2_t::proc_ev_t ev)
{
  tvars {
    str in;
    u_int opts (P_IINFO|P_VERBOSE);
    const mime_type_map_t *mtm;
    str sffx;
    str mime_type;
    str lang;
  }

  static bool first_time = true;
 

  if (first_time)
  {
    // XXX - test that it's OK to write to stdout.  This used to crash
    // the server!
    const char *test = "hello world!\n";
    rc_ignore (write (1, test, strlen (test)));
    first_time = false;
  }

  if (cgi.blookup ("wss")) {
    opts |= P_WSS;
  }

  rc = true;
  static rxx x ("^(:\\d+)?/static(/.+)$");
  in = hdr.get_target ();

  if (!(mtm = ok_static->mime_type_map ())) {
    error (MIME_TYPES_FILE);

  } else {
    
    if (in == "/") {
      ofn = SPLASH_PAGE;
    } else if (x.match (in)) {
      ofn = x[2];
      assert (ofn);
    }

    if (ofn) {

      if ((mime_type = mtm->lookup (ofn, &sffx))) {

	if (cicmp (sffx, "html")) {

	  // XXX - note that this is unsafe vis-a-vis XSS attacks.
	  cgi.load_aarr (&aarr);

	  {
	    str s;
	    if (is_ssl () && (s = ssl_cipher ())) {
	      aarr.add ("CIPHER", s);
	    }
	  }

	} else {
	  opts |= (P_NOPARSE|P_NOLOCALE);
	}
	set_content_type (mime_type);

	if (cgi.lookup ("LANG", &lang)) {
	  set_localizer (ok_static->lfact ()->mk_localizer (lang));
	}
	
	twait { pub2_local ()->run (&out, ofn, mkevent (rc), &aarr, opts); }

	if (!rc) 
	  error (ofn);
	
      } else {
	
	if (sffx) {
	  strbuf b;
	  b << "&lt; <i>no mime-type found for suffix '"
	    << sffx << "'</i> &gt;";
	  error (b);
	} else {
	  error ("&lt; <i>no suffix found on file!</i> &gt;");
	}
      }

    } else {
      error ("&lt; <i>no file specified</i> &gt;");
    }

  }

  if (!rc) {
    twait { pub2 ()->run (&out, ofn, mkevent (rc), &aarr, opts|P_VISERR); }
  }
  twait { output (out, mkevent ()); }
  ev->trigger (true, HTTP_OK);
}

oksrvc_t::newclnt_t *
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
