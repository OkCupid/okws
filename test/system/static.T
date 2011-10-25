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
#include "okclib.h"

//=======================================================================

class oksrvc_static_t : public oksrvc_t {
public:
  oksrvc_static_t (int argc, char *argv[]) 
    : oksrvc_t (argc, argv), _mtm (NULL) {}

  newclnt_t *make_newclnt (ptr<ahttpcon> x);
  void post_launch_pub_T (evb_t cb, CLOSURE);
  void post_launch_pub (evb_t cb) { post_launch_pub_T (cb); }

  const mime_type_map_t *mime_type_map () const { return _mtm; }
  ptr<const std_locale::localizer_factory_t> lfact () const { return _lfact; }

private:
  mime_type_map_t *_mtm;
  ptr<std_locale::localizer_factory_t> _lfact;
};

//=======================================================================

class okclnt_static_t : public okclnt2_t {
public:
  okclnt_static_t (ptr<ahttpcon> x, oksrvc_static_t *o)
    : okclnt2_t (x, o), ok_static (o), _dict (pub3::dict_t::alloc ()) {}
  ~okclnt_static_t () {}

  void process (proc_ev_t ev) { process_T (ev); }
  void process_T (proc_ev_t ev, CLOSURE); 
  bool do_keepalive () { return true; }

protected:
  void error (const str &desc);
  str ofn;
  bool rc;
  oksrvc_static_t *ok_static;
  ptr<pub3::dict_t> _dict;
};

//-----------------------------------------------------------------------

tamed void 
oksrvc_static_t::post_launch_pub_T (evb_t ev)
{
  tvars {
    bool res1, res2, res3;
    pub3::obj_dict_t d;
    pub3::opts_t o (pub3::P_COPY_CONF);
  }
  twait { 
    oksrvc_t::post_launch_pub (mkevent (res1));
    pub3 ()->run_cfg (INTL_CONF_FILE,  mkevent (res2), NULL, o);
    pub3 ()->run_cfg (MIME_TYPES_FILE, mkevent (res3), NULL, o);
  }
  d = pub3 ()->pub3_config_obj ();

  pub3::library_t::import (okclib::lib_t::alloc ());

  if (res3) {
    _mtm = New mime_type_map_t (d("MimeTypes"));
  }
  if (res2) {
    str fmt = d("LocaleFormat").to_str ();
    if (fmt)
      _lfact = std_locale::localizer_factory_t::alloc (fmt);
  }

  ev->trigger (res1 && res2);
}

//-----------------------------------------------------------------------

void
okclnt_static_t::error (const str &s)
{
  _dict->insert ("target", s);
  ofn = FNF_FILE;
  rc = false;
  set_status (HTTP_NOT_FOUND);
}

//-----------------------------------------------------------------------

tamed void 
okclnt_static_t::process_T (okclnt2_t::proc_ev_t ev)
{
  tvars {
    str in;
    bool wss (true);
    pub3::opts_t opts (0);
    const mime_type_map_t *mtm;
    str sffx;
    str mime_type;
    str lang;
    bool skip_pub (false);
    pub3::opts_t oe;
    bool dump_stats (false);
    bool html (true);
    time_t delay (0);
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

  opts |= (pub3::P_STRICT_INCLUDE_SCOPING | pub3::P_WARN_STRICT);
  oe = pub3::P_OUTPUT_ERR_PLACEHOLDERS | pub3::P_OUTPUT_ERR_OBJ;

  if (cgi.blookup ("nowss")) { wss = false; }
  if (cgi.blookup ("viserr")) { opts |= pub3::P_OUTPUT_ERR_IN_PLACE; }
  if (cgi.blookup ("iinfo")) { opts |= pub3::P_IINFO; }
  if (cgi.blookup ("verbose")) { opts |= pub3::P_VERBOSE; }
  if (cgi.blookup ("errobj")) { opts |= oe; }
  if (cgi.blookup ("warninline")) { opts |= pub3::P_WARN_INLINE_NULL; }
  if (cgi.blookup ("utf8json")) { opts |= pub3::P_UTF8_JSON; }
  cgi.lookup ("delay", &delay);
  dump_stats = cgi.blookup ("stats");

  if (delay) { twait { delaycb (delay, 0, mkevent ()); } }

  if (wss) { opts |= pub3::P_WSS; }

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

    if (ofn && ofn == "/null") {
      skip_pub = true;
    } else if (ofn) {

      if ((mime_type = mtm->lookup (ofn, &sffx))) {

	if (cicmp (sffx, "html")) {

	  // XXX - note that this is unsafe vis-a-vis XSS attacks.
	  cgi.load_dict (_dict);

	  {
	    str s;
	    if (is_ssl () && (s = ssl_cipher ())) {
	      _dict->insert ("CIPHER", s);
	    }
	  }

	} else {
	  opts |= (pub3::P_NOPARSE|pub3::P_NOLOCALE);
	  html = false;
	}
	set_content_type (mime_type);

	if (cgi.lookup ("LANG", &lang)) {
	  set_localizer (ok_static->lfact ()->mk_localizer (lang));
	}
	
	twait { pub3_local ()->run (&out, ofn, mkevent (rc), _dict, opts); }

	if (!cgi.blookup ("no_error_page") && !rc) {
	  error (ofn);
	}
	
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
  if (dump_stats) {
    warn << "Headers len: " << inreq_header_len() << "\n";
    warn << "Body len: " << hdr.contlen << "\n";
  }

  if (!rc && !skip_pub) {
    twait { 
      pub3 ()->run (&out, ofn, mkevent (rc), _dict, opts|pub3::P_VISERR); 
    }
  }
  twait { output (out, mkevent ()); }
  if (!html) { 
    twait { delaycb (0, 0, mkevent ()); }
  }
  ev->trigger (true, 0);
}

//-----------------------------------------------------------------------

oksrvc_t::newclnt_t *
oksrvc_static_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_static_t (x, this);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_static_t (argc, argv);
  oksrvc->launch ();

  // if there are bugs in the pub3 runtime, don't kill a 64-bit
  // machine due to swap death!
  struct rlimit rl;
  rlim_t lim = 600000000; // about 600Mb
  rl.rlim_cur = lim;
  rl.rlim_max = lim;
  if (setrlimit (RLIMIT_AS, &rl) < 0) {
    warn ("setrlimit() failed: %m\n");
  }
  amain ();
}
