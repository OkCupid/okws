
#include <sys/param.h>
#include <stdlib.h>
#include "pub_parse.h"
#include "pub.h"
#include "rxx.h"

/* global publishing objects -- when parsing, pub == parser */
pub_t *pub;
pub_parser_t *parser;
pub_client_t *pcli;
pub_base_t *pubincluder;

char dwarnbuf[1024];

bound_pfile_t::~bound_pfile_t ()
{
  if (file && delfile)
    delete file;
}

pbinding_t *
pub_t::set_t::alloc (const pfnm_t &fn, phashp_t h, bool jailed)
{
  pbinding_t *r = New pbinding_t (fn, h, jailed);
  insert (r);
  return r;
}

void
pub_t::set_t::insert (bpfcp_t bpf)
{
  insert (bpf->bnd, bpf->file);
}

void
pub_t::set_t::insert (const pfile_t *f)
{
  assert (!files[f->hsh]);
  files.insert (const_cast<pfile_t *> (f));
  return;
}

void
pub_t::set_t::remove (pfile_t *f)
{
  files.remove (f);
}

void
pub_t::set_t::remove (const pbinding_t *bnd, pfile_t *f)
{
  bindings.unbind (bnd);
  remove (f);
}

void
pub_t::set_t::insert (const pbinding_t *bnd, const pfile_t *f)
{
  bindings.bind (bnd);
  if (f && !(files[bnd->hash ()])) {
    assert (*f->hsh == *(bnd->hash ()));
    files.insert (const_cast<pfile_t *> (f));
  }
}

str
pub_parser_t::jail2real (const str &n) const
{
  str ret;
  if (!n || !behind_bars ())
    ret = n;
  else {
    switch (jm) {
    case JAIL_NONE:
    case JAIL_REAL:
      ret = n;
      break;
    case JAIL_VIRTUAL:
      {
	const char *cp = n.cstr ();
	while (*cp++ == '/') ;
	if (*cp) cp--;
	ret = strbuf (jaildir) << "/" << cp;
	break;
      }
    default:
      break;
    }
  }
  if (opts & P_VERBOSE)
    warn << "jail2real: " << n << " --> " << ret << "\n";
  return ret;
}
  
pbinding_t *
pub_parser_t::to_binding (const pfnm_t &fn, set_t *s, bool toplev)
{
  if (!s) s = set;
  pfnm_t fn2 = complete_fn (fn);
  if (!fn2) return NULL;
  return s->to_binding (fn2, jail2real (fn2), rebind, toplev);
}

pbinding_t *
pub_t::set_t::to_binding (const pfnm_t &fn, const pfnm_t &rfn, bool rebind,
			  bool toplev)
{
  pbinding_t *r = NULL;
  if (rebind) {
    phashp_t h = file2hash (rfn);
    if (h) {
      if (!(r = bindings[fn]) || *r != *h)
	r = alloc (fn, h, toplev);
    }
  } else {
    if (!(r = bindings[fn])) {
      phashp_t h = file2hash (rfn);
      if (h) r = alloc (fn, h, toplev);
    }
  }
  return r;
}

bpfcp_t
pub_parser_t::parse1 (const pbinding_t *bnd, pfile_sec_t *ss, pfile_type_t t)
{
  bpfcp_t r = parse2 (bnd, ss, t);
  if (!r)
    return NULL;
  while (parsequeue.size ()) {
    const pbinding_t *bnd = to_binding (parsequeue.pop_front ());
    parse2 (bnd, New pfile_html_sec_t (0), 
	   wss ? PFILE_TYPE_WH : PFILE_TYPE_H);
  }
  return r;
}

bpfcp_t
pub_parser_t::parse2 (const pbinding_t *bnd, pfile_sec_t *ss, pfile_type_t t)
{
  bpfmp_t r, of;
  const str &fn = bnd->filename ();
  pfile_t *pf = set->files[bnd->hash ()];
  if (pf) {
    r = bound_pfile_t::alloc (bnd, pf);
    if (ss) delete ss;
    r->explore (EXPLORE_PARSE);
  } else {
    pf = New pfile_t (bnd->hash (), t);
    r = bound_pfile_t::alloc (bnd, pf, jail2real (fn));

    if (!r->open ()) 
      goto parse_fail;

    of = bpf; 
    push_file (r);
    if (ss)
      pf->push_section (ss);
    pf->lex_activate (t);
    yyparse ();
    if (of) 
      of->file->lex_reactivate ();
    if (ss)
      pf->add_section ();
    pop_file ();
    
    if (pf->err != PUBSTAT_OK) 
      goto parse_fail;

    r->close ();

    set->insert (pf);
  }
  if (xset_collect)
    xset.push_back (r);
  return r;

 parse_fail:
  PWARN(fn << ": parse failed");
  delete pf; // this will delete ss, too
  return NULL;
  
}

void
pub_parser_t::dump_stack ()
{
  u_int i = 0;
  for (; i < stack.size (); i++) 
    warnx << "\t" << i << ": " << stack[i]->loc ();
  if (bpf)
    warnx << "\t" << i << ": " << bpf->loc ();
}

static rxx dds ("\\.\\./");
pfnm_t 
pub_parser_t::complete_fn (const pfnm_t &fn) const
{
  if (!fn)
    return NULL;
  str ret;
  const char *s1 = fn.cstr ();
  if (dds.search (fn) && (jm == JAIL_VIRTUAL || jm == JAIL_REAL)) {
    ret = NULL;
  } else {
    if (*s1 == '/' || !bpf) {
      ret = fn;
    } else {
      const str &fn2 = bpf->filename ();
      const char *s2 = fn2.cstr ();
      const char *s3 = s2 + fn2.len () - 1;
      while (s3 > s2 && *s3 != '/') s3--;
      str pdir = (*s3 == '/') ? str (s2, s3 - s2 + 1) : str ("/");
      ret = strbuf (pdir) << s1;
    }
  } 
  if (opts & P_VERBOSE) 
    warn << "complete_fn: " << fn << " --> " << (ret ? ret : str ("(null)"))
	 << "\n"; // debug
  return ret;
}

bool
pub_parser_t::init (const str &fn)
{
  pbinding_t *bnd = to_binding (fn);
  if (!bnd)
    return false;
  if (!parse (bnd, PFILE_TYPE_H))
    return false;
  add_rootfile (fn);
  return true;
}


bpfcp_t
pub_t::set_t::getfile (const pfnm_t &nm) const
{
  const pbinding_t *b = bindings[nm];
  if (!b) return NULL;
  pfile_t *f = files[b->hash ()];
  assert (f);
  return bound_pfile_t::alloc (b, f);
}


bool
pub_base_t::include (zbuf *b, const pfnm_t &fn, u_int opt, aarr_t *a) const
{
  bpfcp_t f;
  if (!(f = v_getfile (fn)))
    return false;
  return include (b, f, opt, a);
}

bool
pub_base_t::include (zbuf *b, bpfcp_t f, u_int opt, aarr_t *a) const
{
  output_std_t o (b, f->file);
  penv_state_t *st = genv.start_output (a, fixopts (opts));
  f->output (&o, &genv, f);
  return genv.finish_output (st);
}

bpfcp_t
pub_parser_t::parse (const pbinding_t *bnd, pfile_type_t t)
{
  pfile_sec_t *sec = NULL;
  yywss = wss ? 1 : 0;
  switch (t) {
  case PFILE_TYPE_H:
    if (!bnd->toplev) 
      send_up_the_river ();
    if (wss) t = PFILE_TYPE_WH;
    sec = New pfile_html_sec_t (0);
    break;
  case PFILE_TYPE_CONF:
    sec = New pfile_html_sec_t (0);
    break;
  case PFILE_TYPE_CODE:
    sec = New pfile_code_t (0);
    break;
  case PFILE_TYPE_EC:
    if (wss) t = PFILE_TYPE_WEC;
    break;
  default:
    fatal << "Unrecognized file format given to pub_t::parse\n";
  }
  bpfcp_t r = parse1 (bnd, sec, t);
  grant_conjugal_visit ();
  return r;
}

bool
pub_t::add_rootfile (const pfnm_t &rf, bool conf)
{
  if (conf && !crfbh[rf]) {
    cfgfiles.push_back (rf);
    crfbh.insert (rf);
  } 

  if (!rfbh[rf]) {
    rfbh.insert (rf);
    rootfiles.push_back (rf);
    return true;
  } else {
    return false;
  }
}

void
pub_client_t::explore (const pfnm_t &fn) const
{
  bpfcp_t f = nset->getfile (fn);
  if (!f) {
    f = set->getfile (fn);
    assert (f && *f);
    nset->insert (f);
  }
  f->explore (EXPLORE_FNCALL);
}

bpfcp_t
pub_parser_t::parse_config (const str &fn, bool init)
{
  bpfcp_t f;
  pbinding_t *bnd;
  if (!(bnd = to_binding (fn))) {
    warn << "Could not find configuration file: " << fn << "\n";
    return NULL;
  }
  if (!(f = parse (bnd, PFILE_TYPE_CONF)))
    return NULL;
  if (init) {
    mcf = f->file;
    run_config (mcf);
    defconf = New refcounted<xpub_getfile_res_t> ();
    defconf->set_status (XPUB_STATUS_OK);
    f->file->to_xdr (defconf->file);
  }
  return f;
}

void
pub_parser_t::export_set (xpub_set_t *out)
{
  bhash<phashp_t> hits;
  vec<const pfile_t *> files_tmp;
  u_int lim = xset.size ();
  out->bindings.setsize (lim);
  for (u_int i = 0; i < lim; i++) {
    xset[i]->bnd->to_xdr (&out->bindings[i]);
    if (!hits[xset[i]->bnd->hsh]) {
      hits.insert (xset[i]->bnd->hsh);
      files_tmp.push_back (xset[i]->file);
    }
  }
  lim = files_tmp.size ();
  out->files.setsize (lim);
  for (u_int i = 0; i < lim; i++)
    files_tmp[i]->to_xdr (&out->files[i]);
  xset.clear ();
  xset_collect = false;
}

pub_proxy_t *
pub_proxy_t::alloc ()
{
  pub_proxy_t *r = New pub_proxy_t ();
  pub = NULL;
  pubincluder = r;
  pcli = NULL;
  parser = NULL;
  return r;
}

pub_parser_t *
pub_parser_t::alloc ()
{
  parser = New pub_parser_t ();
  pub = parser;
  pcli = NULL;
  pubincluder = pub;
  return parser;
}

pub_client_t *
pub_client_t::alloc ()
{
  parser = NULL;
  pcli = New pub_client_t ();
  pub = pcli;
  pubincluder = pub;
  return pcli;
}


bpfcp_t
pub_t::getfile (const pfnm_t &fn) const
{
  bpfcp_t bpf = set->getfile (fn);
  if (!bpf) {
    warn << "File lookup failed for filename: " << fn << "\n";
  }
  return bpf;
}

pfile_t *
pub_t::getfile (phashp_t hsh) const
{
  return set->files[hsh];
}

void
pub_parser_t::pop_file ()
{
  if (stack.size ()) {
    bpf = stack.pop_back ();
  } else {
    bpf = NULL;
  }
}

void
pub_parser_t::setjail (str jd, bool permissive)
{
  if (!jd && !(jd = cfg ("JailDir"))) jd = ".";
  if (jd != "." && !isdir (jd)) 
    fatal << jd << ": cannot access jail directory\n";

  if (permissive) {
    jm = JAIL_PERMISSIVE;
  } else if (getuid ()) {
    jm = JAIL_VIRTUAL;
  } else {
    jm = JAIL_REAL;
    if (jd != "." && chroot (jd))
      fatal << jd << ": cannot chroot\n";
  }
  if (jd != "." && (opts & P_VERBOSE))
    warn << "Top (Jail) Directory is: " << jd << "\n";
  jaildir = jd;
}

void
pub_parser_t::setprivs (str jd, str un, str gn)
{
  if (!un && !(un = cfg ("RunAsUser"))) un = ok_uname;
  if (!gn && !(gn = cfg ("RunAsGroup"))) gn = ok_gname;

  // need to call this before chroot'ing! otherwise, we can't open /etc/passwd
  int uid, gid;
  if ((uid = uname2uid (un)) < 0) fatal << un << ": no such user\n";
  if ((gid = gname2gid (gn)) < 0) fatal << gn << ": no such group\n";

  setjail (jd, false);
  send_up_the_river ();

  if (!getuid ()) {
    setgid (gid);
    if (uid) setuid (uid);
  }
}

str 
pub_t::cfg (const str &n) const
{
  ref<pvar_t> v (New refcounted<pvar_t> (n));
  return v->eval (&genv, EVAL_INTERNAL);
}

bool
pub_t::cfg (const str &n, pval_t **v) const
{
  return ((*v = genv.lookup (n, false)));
}

bool
pub_t::cfg (const str &n, str *s) const
{
  return ((*s = cfg (n)));
}

pfnm_t 
pub_t::apply_homedir (const pfnm_t &n) const
{
  return (!n || n.len () <= 0 || !homedir || n[0] == '/') ?
    n : str (strbuf (homedir) << "/" << n);
}

bool
pub_base_t::run_configs ()
{
  genv.clear ();
  run_config (mcf);
  bool ret = true;
  u_int lim = cfgfiles.size ();
  for (u_int i = 0; i < lim; i++) 
    if (!run_config (cfgfiles[i]))
      ret = false;
  return ret;
}

bool
pub_base_t::run_config (str s)
{
  bpfcp_t f;
  if (!(f = v_getfile (s)))
    return false;
  output_conf_t o (true);  // descend recursively = true
  f->output (&o, &genv, f);
  return true;
}

bool
pub_base_t::run_config (pfile_t *f)
{
  if (f) {
    output_conf_t o (false);  // recursive descent = off
    f->output (&o, &genv);
    return true;
  } else
    return false;
}

void
pub_parser_t::push_parr (ptr<parr_t> p)
{
  if (parr)
    parr_stack.push_back (parr);
  parr = p;
}

ptr<parr_t>
pub_parser_t::pop_parr ()
{
  ptr<parr_t> ret = parr;
  if (parr_stack.size ())
    parr = parr_stack.pop_back ();
  return ret;
}

const char *
getenvval (const char *s)
{
  const char *p;
  for (p = s; *p && *p != '='; p++);
  while (isspace (*p++)) ;
  return p;
}

void
pub_base_t::r_config (pubrescb c, int mtd, ptr<aclnt> clnt)
{
  ptr<xpub_getfile_res_t> xr = New refcounted<xpub_getfile_res_t> ();
  clnt->call (mtd, NULL, xr, wrap (this, &pub_base_t::configed, xr, c));
}

void
pub_base_t::configed (ptr<xpub_getfile_res_t> xr, pubrescb c, clnt_stat err)
{
  ptr<pub_res_t> res = New refcounted<pub_res_t> ();
  if (err) {
    res->add (err);
  } else if (xr->status == XPUB_STATUS_NOENT) {
    res->add ("no default configuration available");
  } else if (xr->status == XPUB_STATUS_ERR) {
    res->add (strbuf ("pub config file: ") << *xr->error);
  } else if (xr->status != XPUB_STATUS_OK) {
    res->add ("pub config file: unexpected error");
  } else {
    run_config ((mcf = New pfile_t (*xr->file)));
    v_config_cb (*xr->file);
  }
  (*c) (res);
}

