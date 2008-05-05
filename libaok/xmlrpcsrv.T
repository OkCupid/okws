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

tamed void
okclnt_xmlrpc_t::handle_multicall_T (xml_req_t q, xml_resp_cb_t cb)
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
  u_int optvals[] = { P_DEBUG, P_IINFO, P_VERBOSE, P_VISERR,
		      P_WSS, P_NOPARSE, P_CONFIG, P_CONFIG };
  u_int *vp;
  const char **o;
  for (o = options, vp = optvals; *o; o++, vp++) {
    _pub_options.insert (*o, *vp);
  }
}

static bool
load_args (xml_obj_const_t x, const oksrvc_xmlrpc_base_t *svc,
	   str *fn, u_int *opts, aarr_t *a, str *lang,
	   bool *conf)
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
    if (o.name () && o.name ().len ()) {
      a->add (o.name (), str (o.value ()));
    }
  }

  str l = x(LANG);
  if (l && l.len () > 0) 
    *lang = l;

  *conf = x(CONF) || x(CONFIG) || (*opts & P_CONFIG);

  return (*fn && fn->len () > 0);
}

#define XML_PUB_ARGLOAD_FAILED  401

static void
pval2xml (xml_obj_ref_t o, pval_t *v, penv_t *env)
{
  if (!v) 
    return;

  const parr_mixed_t *ma;
  const parr_ival_t *ia;
  int64_t t;
  
  if ((ma = v->to_mixed_arr ())) {
    for (size_t i = 0; i < ma->size (); i++) {
      pval2xml (o[i], (*ma)[i], env);
    }
  } else if ((ia = v->to_int_arr ())) {
    for (size_t i = 0; i < ia->size (); i++) {
      int t = 0;
      ia->val (i, &t);
      o[i] = t;
    }
  } else if (v->to_int64(&t)) {
    o = int (t);
  } else {
    // everything should be flattened anyways....
    o = v->eval (env, EVAL_INTERNAL);
  }
}

static xml_obj_t
aarr2xml (ptr<aarr_t> a, penv_t *env)
{
  xml_obj_t ret (New refcounted<xml_struct_t> ());
  if (a) {
    const nvtab_t *nvt = a->nvtab ();
    nvpair_t *p;
    for (p = nvt->first (); p; p = nvt->next (p)) {
      pval2xml ( ret(p->name()), p->value (), env);
    }
  }
  return ret;
}

tamed void
okclnt_xmlrpc_t::handle_okws_pub_T (xml_req_t q, xml_resp_cb_t cb)
{
  tvars {
    xml_resp_t r;
    size_t i;
    str fn;
    u_int opts;
    vec<ptr<zbuf> > out;
    vec<ptr<aarr_t> > conf_out;
    vec<xpub_status_t> pub_errc;
    aarr_t aa;
    vec<size_t> calls;
    ptr<bound_pfile2_t> dummy;
    str lang;
    bool conf;
    ptr<pub2::ok_iface_t> iface;
    penv_t *env;
  }

  // need an env for conf files ?
  env = pub2 ()->get_env ();

  r.setsize (q.size ());
  out.setsize (q.size ());
  conf_out.setsize (q.size ());
  pub_errc.setsize (q.size ());

  twait {
    for (i = 0; i < q.size (); i++) {
      aa.deleteall ();
      if (load_args (q[i], _srvc, &fn, &opts, &aa, &lang, &conf)) {
	calls.push_back (i);
	iface = xml_pub2_local (lang);
	if (conf) {
	  conf_out[i] = New refcounted<aarr_t> ();
	  iface->run_cfg_full (fn, mkevent (pub_errc[i], dummy),
			       conf_out[i]);
	} else {
	  out[i] = New refcounted<zbuf> ();
	  iface->run_full (out[i], fn, 
			   mkevent (pub_errc[i], dummy), 
			   &aa, opts);
	}
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
    } else if (out[j]) {
      strbuf b ;
      out[j]->to_strbuf (&b, false);
      r[j]("data") = base64_str_t (b);
    } else {
      assert (conf_out[j]);
      r[j]("data") = aarr2xml (conf_out[j], env);
    }
  }
  TRIGGER (cb, r);
}

#endif /* HAVE_EXPAT */
