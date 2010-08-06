// -*-c++-*-

#include "xmlrpcsrv.h"
#include "parseopt.h"
#include "pub3.h"

#ifdef HAVE_EXPAT

void
okclnt_xmlrpc_base_t::reply (xml_resp_t r)
{
  set_content_type ("text/xml");
  out << "<?xml version=\"1.0\"?>\n";
  r.output (out);
  output (out);
}

u_int64_t xml_rpc_id = 0;

u_int64_t
xml_rpc_new_global_id ()
{
  return ++xml_rpc_id;
}

tamed void
okclnt_xmlrpc_t::handle_multicall_T (xml_req_t q, xml_resp_ev_t cb)
{
  tvars {
    size_t i;
    str nm;
    vec<xml_resp_t> r2;
    xml_resp_t r;
    vec<size_t> calls;
    ptr<const xml_container_t> xc;
  }

  r2.setsize (q.size ());

  twait {
    for (i = 0; i < q.size (); i++) {
      if (!(nm = q[i]("methodName"))) {
	r[i] = xml_fault_obj_t (OK_XMLRPC_ERR_NO_METHOD_CALL,
			     "No methodCall for multicall call");
      } else if (nm == MULTICALL) {
	r[i] = xml_fault_obj_t (OK_XMLRPC_ERR_RECURSIVE,
			     "Cannot make recursive multicalls");
      } else if (!(xc = q[i]("params").to_xml_container ())) {
	r[i] = xml_fault_obj_t (OK_XMLRPC_ERR_BAD_PARAMS,
				"Bad parameters to multicall");
      } else {
	calls.push_back (i);
	_srvc->handle (this, nm, xml_req_t (xc), mkevent (r2[i]));
      }
    }
  }

  for (i = 0; i < calls.size (); i++)
    r[calls[i]][0] = r2[calls[i]];

  cb->trigger (r);
}

void
okclnt_xmlrpc_t::handle_set_debug_level (xml_req_t q, xml_resp_ev_t cb)
{
  xml_resp_t r;
  r[0] = true;
  int i = q[0];
  _srvc->set_debug_level (i);
  cb->trigger (r);
}

void
oksrvc_xmlrpc_base_t::set_debug_level ()
{
  int lev;
  const char *e;  
  if ((e = safegetenv ("XMLRPC_SRV_TRACE")) && convertint (e, &lev)) 
    set_debug_level (lev);
}

void
oksrvc_xmlrpc_base_t::set_debug_level (int i)
{
  if (i > 0) {
    warn << "debug level changed to " << i << "\n";
    _debug_level = i;
  }
}

void
oksrvc_xmlrpc_base_t::do_debug (ptr<const xml_element_t> el,
				const char *m, u_int64_t i)
{
  if (_debug_level > 2) {
    strbuf b;
    b << m << "; ID=" << i;
    if (_debug_level >= 10) {
      if (_debug_level >= 20) {
	zbuf z;
	el->dump (z);
	b << "\n";
	z.to_strbuf (&b, false);
      } else {
	b << ": ";
	el->dump_to_python (b);
	b << "\n";
      }
    } else {
      b << "\n";
    }
    b.tosuio ()->output (2);
  }
}

void 
oksrvc_xmlrpc_base_t::init_optmap ()
{
  const char *options[] = { "debug", "includeInfo", "verbose",
			    "visibleErrors", "WSS", "raw", 
			    "conf", "config", NULL };
  u_int optvals[] = { pub3::P_DEBUG, 
		      pub3::P_IINFO, 
		      pub3::P_VERBOSE, 
		      pub3::P_VISERR,
		      pub3::P_WSS, 
		      pub3::P_NOPARSE, 
		      pub3::P_CONFIG, 
		      pub3::P_CONFIG };
  u_int *vp;
  const char **o;
  for (o = options, vp = optvals; *o; o++, vp++) {
    _pub_options.insert (*o, *vp);
  }
}

static bool
load_args (xml_obj_const_t x, const oksrvc_xmlrpc_base_t *svc,
	   str *fn, pub3::opts_t *opts, str *lang,
	   bool *conf, pub3::obj_t *p3o)
{
  static const char *FN = "filename";
  static const char *OPTS = "options";
  static const char *VARS = "variables";
  static const char *LANG = "language";
  static const char *CONF = "conf";
  static const char *CONFIG = "config";
  
  *fn = x(FN);
  *opts = 0;
  const u_int *opt;
  str s = x(OPTS);
  if (s && s.len ()) {
    if ((opt = svc->lookup_opt (s))) {
      *opts |= (*opt);
    }
  } else {
    for (size_t j = 0; j < x(OPTS).size (); j++) {
      if ((opt = svc->lookup_opt (x(OPTS)[j]))) {
	*opts |= (*opt);
      }
    }
  }
  for (size_t j = 0; j < x(VARS).size (); j++) {
    xml_obj_const_t o = x(VARS)[j];
    str nm = o.name ();
    if (nm && nm.len ()) {
	o.to_pub3 (p3o);
    }
  }

  str l = x(LANG);
  if (l && l.len () > 0) 
    *lang = l;

  *conf = x(CONF) || x(CONFIG) || (*opts & pub3::P_CONFIG);

  return (*fn && fn->len () > 0);
}

#define XML_PUB_ARGLOAD_FAILED  401

tamed void
okclnt_xmlrpc_t::handle_okws_pub_T (xml_req_t q, xml_resp_ev_t cb)
{
  tvars {
    xml_resp_t r;
    size_t i;
    str fn;
    pub3::opts_t opts;
    ptr<zbuf> zb;
    str lang;
    bool conf;
    ptr<pub3::ok_iface_t> iface;
    pub3::obj_t o;
    bool ok;
    ptr<pub3::dict_t> conf_out;
    pub3::status_t status;
  }

  r.setsize (q.size ());

  for (i = 0; i < q.size (); i++) {
    if (load_args (q[i], _srvc, &fn, &opts, &lang, &conf, &o)) {
      iface = xml_pub3_local (lang);
      ok = true;
      status.set_status (XPUB_STATUS_OK);
      if (conf) {
	conf_out = pub3::dict_t::alloc ();
	twait { iface->run_cfg (fn, mkevent (ok), conf_out, opts, &status); }
	if (ok) {
	  xml_obj_t tmp = xml_obj_t::from_pub3 (conf_out);
	  r[i]("data") = tmp.el ();
	}
      } else {
	zb = New refcounted<zbuf> ();
	twait { iface->run (zb, fn, mkevent (ok), o.dict (), opts, &status); }
	if (ok) {
	  strbuf b ;
	  zb->to_strbuf (&b, false);
	  r[i]("data") = base64_str_t (b);
	}
      }
      if (!ok) {
	r[i]("error")("code") = int (status.status);
	r[i]("error")("desc") = errcode2str (status);
      }
    } else {
      r[i]("error")("code") = XML_PUB_ARGLOAD_FAILED;
      r[i]("error")("desc") = "failed to load Pub arguments";
    }
  }
  cb->trigger (r);
}

#endif /* HAVE_EXPAT */
