// -*-c++-*-
/* $Id: purify.T 3190 2008-02-05 15:10:03Z max $ */

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
#include "okwcxml.h"
#include "tame_connectors.h"

class oksrvc_purify_t : public oksrvc_t {
public:
  oksrvc_purify_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
  void post_launch_pub2 (cbb cb) { post_launch_pub2_T (cb); }
  ptr<okwc3::agent_xml_t> agent() { return _cli; }
protected:
  void post_launch_pub2_T (cbb cb,  CLOSURE);
  ptr<okwc3::agent_xml_t> _cli;
};

class okclnt_purify_t : public okclnt_t {
public:
  okclnt_purify_t (ptr<ahttpcon> x, oksrvc_purify_t *o)
    : okclnt_t (x, o), ok_purify (o) {}
  ~okclnt_purify_t () {}

  void process () { process_T (); }
  void process_T (CLOSURE); 

protected:
  oksrvc_purify_t *ok_purify;
};

static rxx url_rxx ("http://([^:/]+)(:(\\d+)/)?(.*)");

tamed void 
oksrvc_purify_t::post_launch_pub2_T (cbb cb)
{
  tvars {
    bool res1, res2;
  }
  twait { 
    oksrvc_t::post_launch_pub2 (mkevent (res1));
    pub2 ()->run_cfg (PURIFY_CONF_FILE, mkevent (res2)); 
  }

  { 
    str url, hn, port_str, path;
    int port (80);

    url = (*pub2())["purify-url"];

    if (url && url_rxx.match (url)) {

      hn = url_rxx[1];
      port_str = url_rxx[3];
      path = url_rxx[4];

      if (port_str)
	convertint (port_str, &port);

      _cli = New refcounted<okwc3::agent_xml_t> (hn, port, path);

    }
  }

  TRIGGER (cb, res1 && res2);
}


tamed void 
okclnt_purify_t::process_T ()
{
  tvars {
    aarr_t aarr;
    bool rc (true);
    u_int opts (P_IINFO|P_VERBOSE);
    str file ("/test/purify.html");
    str dirtyhtml;
    str outhtml;
    xml_outreq_t req;
    xml_inresp_t resp;
    int status;
    int tmo (20);
    int code;
    str msg;
    ptr<okwc3::agent_xml_t> cli;
  }

  dirtyhtml = cgi["dirtyhtml"];
  if (cgi.blookup ("doit")) {
    outhtml = "<N/A> -- Failure!";

    if ((cli = ok_purify->agent ())) {
      req.set_method_name ("html.purify");
      req[0] = base64_str_t (dirtyhtml);
      status = HTTP_TIMEOUT;
      twait { 
	cli->call (req, connector::timeout (mkevent (status, resp), tmo));
      }
      if (status == HTTP_OK) {
	if (resp.is_fault (&code, &msg)) {
	  warn << "Fault(" << code << "): ";
	  if (msg) warnx << msg;
	  warnx << "\n";
	} else {
	  strbuf b;
	  resp.output_python (b);
	  warn << b << "\n";
	  outhtml = resp[0];
	}
      } else {
	warn << "xml rpc failure: " << status << "\n";
      }
    } else {
      warn << "no agent available\n";
    }
  } else {
    outhtml = dirtyhtml;
  }
  aarr.add ("out", outhtml);

  twait { pub2 ()->run (&out, file, mkevent (rc), &aarr, opts|P_VISERR); }
  output (out);
}

oksrvc_t::newclnt_t *
oksrvc_purify_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_purify_t (x, this);
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_purify_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
