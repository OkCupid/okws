// -*-c++-*-

#include "xmlrpcsrv.h"
#include "parseopt.h"

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

TAMED void
okclnt_xmlrpc_t::handle_mutlicall_T (xml_req_t q, xml_resp_cb_t cb)
{
  VARS {
    size_t i;
    str nm;
    vec<xml_resp_t> r2;
    xml_resp_t r;
    vec<size_t> calls;
    ptr<const xml_container_t> xc;
  }

  r2.setsize (q.size ());

  BLOCK {
    for (i = 0; i < q.size (); i++) {
      if (!(nm = q[i]("methodName"))) {
	r[i] = xml_fault_obj_t (OK_XMLRPC_ERR_NO_METHOD_CALL,
			     "No methodCall for mutlicall call");
      } else if (nm == MULTICALL) {
	r[i] = xml_fault_obj_t (OK_XMLRPC_ERR_RECURSIVE,
			     "Cannot make recursive multicalls");
      } else if (!(xc = q[i]("params").to_xml_container ())) {
	r[i] = xml_fault_obj_t (OK_XMLRPC_ERR_BAD_PARAMS,
				"Bad parameters to mutlicall");
      } else {
	calls.push_back (i);
	_srvc->handle (this, nm, xml_req_t (xc), @(r2[i]));
      }
    }
  }

  for (i = 0; i < calls.size (); i++)
    r[calls[i]][0] = r2[calls[i]];

  (*cb) (r);
}

void
okclnt_xmlrpc_t::handle_set_debug_level (xml_req_t q, xml_resp_cb_t cb)
{
  xml_resp_t r;
  r[0] = true;
  int i = q[0];
  _srvc->set_debug_level (i);
  (*cb) (r);
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
    }
    b.tosuio ()->output (2);
  }
}

void 
oksrvc_xmlrpc_base_t::init_optmap ()
{
  const char *options[] = { "debug", "includeInfo", "verbose",
			    "visibleErrors", "WSS", "raw", NULL };
  u_int optvals[] = { P_DEBUG, P_IINFO, P_VERBOSE, P_VISERR,
		      P_WSS, P_NOPARSE };
  u_int *vp;
  const char **o;
  for (o = options, vp = optvals; *o; o++, vp++) {
    _pub_options.insert (*o, *vp);
  }
}

static bool
load_args (xml_obj_const_t x, const oksrvc_xmlrpc_base_t *svc,
	   str *fn, u_int *opts, aarr_t *a)
{
  static const char *FN = "filename";
  static const char *OPTS = "options";
  static const char *VARS = "variables";
  
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
    if (o.name () && o.name ().len ()) {
      a->add (o.name (), str (o.value ()));
    }
  }
  
  return (*fn && fn->len () > 0);
}

#define XML_PUB_ARGLOAD_FAILED  401

TAMED void
okclnt_xmlrpc_t::handle_okws_pub_T (xml_req_t q, xml_resp_cb_t cb)
{
  VARS {
    xml_resp_t r;
    size_t i;
    str fn;
    u_int opts;
    vec<ptr<zbuf> > out;
    vec<xpub_status_t> pub_errc;
    aarr_t aa;
    vec<size_t> calls;
    ptr<bound_pfile2_t> dummy;
  }
  BLOCK {
    r.setsize (q.size ());
    out.setsize (q.size ());
    pub_errc.setsize (q.size ());
    for (i = 0; i < q.size (); i++) {
      out[i] = New refcounted<zbuf> ();
      aa.clear ();
      if (load_args (q[i], _srvc, &fn, &opts, &aa)) {
	calls.push_back (i);
	pub2 ()->run_full (out[i], fn, @(pub_errc[i], dummy), &aa, opts);
      } else {
	r[i]("error")("code") = XML_PUB_ARGLOAD_FAILED;
	r[i]("error")("desc") = "failed to load Pub arguments";
      }
    }
  }
  for (i = 0; i < calls.size (); i++) {
    size_t j = calls[i];
    if (pub_errc[j].status != XPUB_STATUS_OK) {
      r[j]("error")("code") = int (pub_errc[j].status);
      r[j]("error")("desc") = errcode2str (pub_errc[j]);
    } else {
      strbuf b ;
      out[j]->to_strbuf (&b, false);
      r[j]("data") = base64_str_t (b);
    }
  }
  SIGNAL (cb, r);
}

#endif /* HAVE_EXPAT */
