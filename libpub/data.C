/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
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

#include "pub.h"
#include "parr.h"
#include "pub_parse.h"
#include "parseopt.h"

pstr_t::pstr_t (ptr<pvar_t> v) : n (0) { add (v); }
pstr_t::pstr_t (const str &s) : n (0) { add (s); }
void pstr_t::output (output_t *o, penv_t *e) const { els.output (o, e); }
void pstr_t::add (ptr<pvar_t> v) { add (New pstr_var_t (v)); n++; }
void pstr_t::add (pstr_el_t *v) { els.insert_tail (v); n++; }
void pfile_set_func_t::dump2 (dumper_t *d) const { if (aarr) aarr->dump (d); }
aarr_t *pswitch_env_base_t::env () const { return aarr; }
void bound_pfile_t::explore (pub_exploremode_t m) const
{ if (file) file->explore (m); }
str penv_t::loc (int l) const 
{ return file ? file->loc (l) : str("at top level"); }
pfnm_t penv_t::filename () const { return file->bnd->filename (); }
void bound_pfile_t::close () { file->close (); }
output_std_t::output_std_t (zbuf *o, const pfile_t *t) :
  output_t (t ? t->type () : PFILE_TYPE_H), out (o), osink_open (false) {}
output_conf_t::output_conf_t (bool d) 
  : output_t (PFILE_TYPE_CONF), env (New refcounted<aarr_t> ()), dflag (d) {}
void pbuf_t::add (zbuf *z) { add (New pbuf_zbuf_t (z)); }
void pval_zbuf_t::eval_obj (pbuf_t *ps, penv_t *e, u_int d) const 
{ ps->add (zb); }
void penv_t::safe_push (ptr<const aarr_t> a) 
{ estack.push_back (a); hold.push_back (a); }

static void
explore (pub_exploremode_t mode, const pfnm_t &nm)
{
  switch (mode) {
  case EXPLORE_PARSE:
    pub->queue_hfile (nm);
    break;
  case EXPLORE_FNCALL:
    pub->explore (nm);
    break;
  case EXPLORE_CONF:
  default:
    break;
  }
}

pub_evalmode_t 
penv_t::init_eval (pub_evalmode_t m)
{
  evaltab.clear ();
  pub_evalmode_t r = evm;
  evm = m;
  return r;
}

void 
pfile_include_t::explore (pub_exploremode_t mode) const
{
  ::explore (mode, fn);
}

void
pfile_inclist_t::explore (pub_exploremode_t mode) const
{
  u_int lim = files.size ();
  for (u_int i = 0; i < lim; i++)
    ::explore (mode, files[i]);
}

void
penv_t::warning (const str &s) const
{
  warn << loc () << ": " << s << "\n";
}

void
penv_t::compile_err (const str &s)
{
  warn << loc () << ": evaluation error: " << s << "\n";
  cerrflag = true;
}

void 
pfile_var_t::output (output_t *o, penv_t *e) const
{
  e->setlineno (lineno);
  var->output (o, e);
  e->unsetlineno ();
}

void
penv_t::resize (size_t s)
{
  assert (s <= estack.size ());
  while (s != estack.size ()) 
    estack.pop_back (); // don't delete!!
}

void
penv_t::gresize (size_t gvs)
{
  assert (gvs <= estack.size ());
  while (gvs != gvars.size ())
    gvars.pop_back (); // don't delete!!
}

void
pfile_frame_t::push_frame (penv_t *p, aarr_t *f, const gvars_t *g) const
{
  mark_frame (p);
  if (f) p->push (f);
  if (g) p->push (g);
}

void
pstr_t::add (const str &s)
{
  pstr_el_t *p = els.last;
  if (!p || !p->concat (s)) {
    els.insert_tail (New pstr_str_t (s));
    n++;
  }
}

void
pfile_html_el_t::output (output_t *o, penv_t *e) const
{
  o->output (e, to_zstr (), true);
}

void
pfile_raw_el_t::output (output_t *o, penv_t *e) const
{
  o->output (e, _dat, true);
}

void
pstr_t::add (char c)
{
  pstr_el_t *p = els.last;
  if (!p || !p->concat (c)) {
    els.insert_tail (New pstr_str_t (c));
    n++;
  }
}

pfile_html_sec_t *
pfile_html_sec_t::add (const str &s)
{
  pfile_el_t *p = els->last;
  if (!p || !p->concat (s))
    els->insert_tail (New pfile_html_el_t (s));
  return this;
}

pfile_html_sec_t *
pfile_html_sec_t::add (char c)
{
  if (nlgobble) {
    nlgobble = false;
    if (c == '\n')
      return this;
  }
  pfile_el_t *p = els->last;
  if (!p || !p->concat (c))
    els->insert_tail (New pfile_html_el_t (c));
  return this;
}

pfile_sec_t *
pfile_sec_t::add (pfile_el_t *el, bool combine)
{
  if (!els)
    return this;
  if (combine) {
    pfile_el_t *p = els->last;
    if (p && el->same_type_as (p) && p->concat (el)) {
      delete el;
      return this;
    } else if (el->sec_traverse_init ()) {
      pfile_el_t *p2;
      while ((p2 = el->sec_traverse ()))
	add (p2, true);
      delete el;
      return this;
    }
  }
  els->insert_tail (el);
  return this;
}

pfile_el_t *
pfile_pstr_t::sec_traverse ()
{
  pstr_el_t *e = pstr->shift ();
  if (!e) return NULL;
  pfile_el_t *r = e->to_pfile_el ();
  delete e;
  return r;
}

pstr_el_t *
pstr_t::shift ()
{
  if (n == 0 || !els.first)
    return NULL;
  n--;
  return els.pop_front ();
}

pfile_sec_t *
pfile_sec_t::add (pfile_sec_t *s)
{
  pfile_el_t *h = s->els->pop_front ();
  if (h) {
    add (h);
    els->append_list (s->els);
  }
  delete s;
  return this;
}

void
pfile_func_t::include (output_t *o, penv_t *g, aarr_t *e, const pfnm_t &nm)
  const
{
  bpfcp_t f;
  if (!o->descend ())
    return;

  if ((f = pub->getfile (nm))) {
    push_frame (g, e);
    bool tlf = g->set_tlf (false);
    o->output_info (g, strbuf ("include: ") << f->filename (), lineno);
    f->output (o, g);
    o->output_info (g, strbuf ("/include: ") << f->filename (), lineno);
    g->set_tlf (tlf);
    pop_frame (o, g);
  } else {
    o->output_err (g, strbuf (nm) << ": cannot include file", lineno);
  }
}

void
pfile_include_t::output (output_t *o, penv_t *genv) const
{
  if (!fn) {
    o->output_err (genv, "include: no file given", lineno);
  } else {
    include (o, genv, env, fn);
  }
}

void
pfile_include2_t::output (output_t *o, penv_t *genv) const
{
  o->output_err (genv, "include: can't call output on v2 include file", 
		 lineno);
}

void
bound_pfile_t::output (output_t *o, penv_t *genv) const
{
  bpfcp_t rct = mkref (const_cast<bound_pfile_t *> (this));
  if (!genv->i_stack_add (rct))
    o->output_err_stacktrace (genv, "circular include detected", 0);
  else { 
    file->output (o, genv);
    genv->i_stack_remove (rct);
  }
}

void
pfile_t::output (output_t *o, penv_t *genv) const
{
  o->output_header (genv);
  secs.output (o, genv);
}

void
penv_t::push_file (bpfcp_t f)
{
  if (file) {
    fstack.push_back (file);
  }
  file = f;
}

void
penv_t::pop_file ()
{
  if (fstack.size ()) {
    file = fstack.pop_back ();
  } else {
    file = NULL;
  }
}

void
pvar_t::output (output_t *o, penv_t *e) const
{
  ptr<pbuf_t> st = eval_to_pbuf (e, EVAL_FULL);
  if (e->cerr) {
    o->output_err (e, e->cerr); // lineno set from within pfile_var_t::output
    return;
  }
  st->output (o, e);
}

ptr<pbuf_t>
evalable_t::eval_to_pbuf (penv_t *e, pub_evalmode_t m) const
{
  ref<pbuf_t> st (New refcounted<pbuf_t>);
  pub_evalmode_t om = EVAL_FULL;
  if (e) om = e->init_eval (m);
  eval_obj (st, e, P_INFINITY);
  if (e) e->set_evalmode (om);
  return st;
}

str
evalable_t::eval (penv_t *e, pub_evalmode_t m, bool allownull) const 
{
  if (m == EVAL_SIMPLE || !e) 
    return eval_simple ();
  ptr<pbuf_t> st = eval_to_pbuf (e, m);
  return st->to_str (m, allownull);
}

void
pfile_switch_t::output (output_t *o, penv_t *e) const
{
  ptr<pswitch_env_base_t> pse = eval_for_output (o, e);
  // we might have given switch an empty file (so as to allow 
  // the default to catch more stuff, for instance).
  if (pse)
    if (pse->fn)
      include (o, e, pse->env (), pse->fn);
    else if (pse->nested_env ()) {
      pse->nested_env ()->output (o, e);
    }
}

void
nested_env_t::output (output_t *o, penv_t *e) const
{
  // Actually allow set operations to persist past the scope of
  // this switch.
  //_frm.mark_frame (e);
  _sec->output (o, e);
  //_frm.pop_frame (o, e);
}

ptr<pswitch_env_base_t>
pfile_switch_t::eval_for_output (output_t *o, penv_t *e) const
{
  str v;
  ptr<pswitch_env_base_t> pse;
  if (key)
    // treading lightly here; adding the "allownull" hack because
    // i'm not sure what kind of semantics people are currently 
    // using.
    v = key->eval (e, EVAL_INTERNAL, true);

  if (!v) {
    pse = nullcase;
    if (!pse) pse = def;

    if (!pse && !nulldef) 
      o->output_err (e, strbuf ("switch: cannot evaluate key value (")
		     << key->name () << ")", lineno);
  } else {
   
    const ptr<pswitch_env_base_t> *psep = _exact_cases[v];
    if (psep)
      pse = *psep;

    if (!pse) {
      scalar_obj_t so (v);
      for (size_t i = 0; i < _other_cases.size () && !pse; i++) {
	if (_other_cases[i]->match (so))
	  pse = _other_cases[i];
      }
    }

    if (!pse) pse = def;

    //
    // MK 12.20.06
    // Used to be this:
    //
    //   if (!pse && (v.len () != 0 || !nulldef)) 
    //
    // Which was causing warnings when some value was given for the key,
    // and a nulldef was given.  I'm not entirely sure what I had in
    // mind with this, but err on the side of fewer warnings here.
    //
    if (!pse && !nulldef)
      o->output_err (e, strbuf ("switch: no case when ") << key->name ()  
		     << " = " << v, lineno);
  } 
  return pse;
}

void
nvtab_t::insert (nvpair_t *p)
{
  nvpair_t *op = (*this)[p->name ()];
  if (op) {
    remove (op);
    delete op;
  }
  super_t::insert (p);
}

void
aarr_t::add (nvpair_t *p)
{
  aar.insert (p);
}

void
gcode_t::eval_obj (pbuf_t *ps, penv_t *e, u_int d) const
{
  if (e->get_tlf ()) 
    ps->add (New pbuf_var_t (nm));
  else {
    str s = strbuf ("@{") << nm << "}";
    ps->add (s);
  }
}

void
parr_mixed_t::eval_obj (pbuf_t *ps, penv_t *e, u_int d) const
{
  u_int lim = v.size ();
  ps->add ("ARR=(");
  for (u_int i = 0; i < lim; i++) {
    if (i != 0)
      ps->add (", ");
    v[i]->eval_obj (ps, e, d);
  }
  ps->add (")");
}

void
pub_regex_t::eval_obj (pbuf_t *ps, penv_t *e, u_int d) const
{
  ps->add (_rxx_str ? _rxx_str : str ("<Empty RXX>") );
}

void
pvar_t::eval_obj (pbuf_t *ps, penv_t *e, u_int d) const
{
  if (e->go_g_var (nm)) {
    ps->add (New pbuf_var_t (nm));
    return;
  }
  const pval_t *pv;
  // for internal pvar_t (for instance, config variables), the pval_t was 
  // specified at initialization; this is a bit of a hack, but it should 
  // work.  Note that also because the initial lookup was without recursion,
  // we need to add the previous stackframe to the eval stack manually,
  // as is done here.
  bool popit = false;
  if (val) {
    pv = val;
  } else {
    // second parameter => true by default, therefore there will be
    // recursion.
    pv = e->lookup (nm);
    popit = true;
  }

  if (pv) {
    pv->eval_obj (ps, e, d);
  } else if (e->getopt (P_COMPILE)) {
    e->compile_err (strbuf ("Cannot resolve variable: ") << nm.cstr ());
  } else if (e->debug ()) {
    e->setlineno (lineno);
    e->warning (strbuf ("cannot resolve variable: " ) << nm.cstr ());
    ps->add (strbuf ("<!--UNDEF: ") << nm << " -->");
    e->unsetlineno ();
  }
  if (popit)
    e->eval_pop (nm);
}

bool
penv_t::go_g_var (const str &n) const
{
  switch (evm) {
  case EVAL_FULL:
  case EVAL_INTERNAL:
    return false;
  default:
    return is_gvar (n);
  }
}

const pval_t *
aarr_t::lookup (const str &n) const
{
  const pval_t *ret = NULL;
  const nvpair_t *p = aar [n];
  if (p) ret = p->value ();
  return ret;
}

pval_t *
aarr_t::lookup (const str &n) 
{
  pval_t *ret = NULL;
  nvpair_t *p = aar [n];
  if (p) ret = p->value ();
  return ret;
}

//
//
// we can only evaluate a variable at a given stack level once; this
// way we cut off any infinite recursion.  we enforce these rules with
// the following function.  once a variable n is evaluated at level
// d, the value (d-1) is pushed on to its vector.  all future evaluations
// within the evaluation attempt will then start at level d-1.
//
const pval_t *
penv_t::lookup (const str &n, bool recurse)
{
  ssize_t i;
  vec<ssize_t> *v = NULL;
  
  if (recurse)
    v = evaltab[n];
  
  i = v ? v->back () : estack.size () -1;
  
  const pval_t *ret = NULL;
  
  for ( ; i >= 0; i--) {
    if ((ret = estack[i]->lookup (n))) 
      break;
  }
  i--;
  
  if (v) 
    v->push_back (i);
  else if (recurse) {
    vec<ssize_t> vv;
    vv.push_back (i);
    evaltab.insert (n, vv);
  }
  return ret;
}

void
penv_t::eval_pop (const str &n)
{
  vec<ssize_t> *v = evaltab[n];
  assert (v);
  if (v->size () == 1)
    evaltab.remove (n);
  else 
    v->pop_back ();
}

static void
_eval (pbuf_t *s, penv_t *e, u_int d, pstr_el_t *el)
{
  el->eval_obj (s, e, d);
}

void
pstr_t::eval_obj (pbuf_t *ps, penv_t *e, u_int d) const
{
  if (els.first) 
    els.traverse (wrap (_eval, ps, e, d));
  else
    ps->add ("");
}

str
pstr_t::eval_simple () const
{
  if (n == 0)
    return "";
  if (n != 1)
    return NULL;
  str r;
  if (!els.first || !(r = els.first->to_str ()))
    return NULL;
  return r;
}

str
parr_mixed_t::eval_simple () const
{
  if (v.size () > 0) return v[0]->eval_simple ();
  else return NULL;
}

void
pfile_gs_t::output (output_t *o, penv_t *e) const
{
  frm.mark_frame (e);
  e->aarr_n = 1;
  pfile_type_t m = o->switch_mode (PFILE_TYPE_GUY);
  els->output (o, e);
  o->switch_mode (m);
  e->needloc = true;
  frm.pop_frame (o, e);
}

pfile_t::pfile_t (const phashp_t &h, pfile_type_t t)
  : err (PUBSTAT_OK), hsh (h), lineno (1), yybs (NULL), fp (NULL),
    pft (t), section (NULL) {}

bool
bound_pfile_t::open ()
{
  FILE *fp;
  struct stat sb;
  if (stat (rfn.cstr (), &sb) != 0 || !S_ISREG (sb.st_mode) ||
      (!(fp = fopen (rfn.cstr (), "r")))) {
    PWARN(rfn << ": cannot open file");
    return false;
  } else {
    file->setfp (fp);
    return true;
  }
}

str
bound_pfile_t::read () const
{
  return file2str (rfn);
}

str
bound_pfile_t::loc (int l) const
{
  strbuf r (filename ());
  r << ":" << (l >= 0 ? l : file->get_lineno ());
  return r;
}

void
pfile_t::setfp (FILE *f)
{
  fp = f;
  yybs = yycreatebuf (fp);
}

void
pfile_t::lex_activate (pfile_type_t t)
{
  yyesc = 1;
  yyswitch (yybs);
  yy_push_pubstate (t);
}

void
pfile_t::lex_reactivate ()
{
  yy_pop_pubstate ();
  yyswitch (yybs);
}

pfile_sec_t *
pfile_t::pop_section ()
{
  pfile_sec_t *r = section;
  section = stack.size () ? stack.pop_back () : NULL;
  return r;
}

void
pfile_t::push_section (pfile_sec_t *s)
{
  if (section)
    stack.push_back (section);
  section = s;
}

bool
concatable_str_t::concat (const str &s)
{
  sb << s; 
  hold.push_back (s); 
  return true; 
}

bool
concatable_str_t::concat (concatable_t *l)
{
  str s = l->to_str ();
  if (!s)
    return false;
  concat (s);
  return true;
}

bool
pfile_g_init_pdl_t::add (ptr<arglist_t> l)
{
  if (l->size () != 1 || !(*l)[0]->is_null ()) {
    PWARN("init_publist () takes no arguments");
    err = true;
  }
  return (!err);
}

bool
pfile_inclist_t::add (ptr<arglist_t> l)
{
  str fn;
  pbinding_t *b = NULL;
  if (!err) {
    u_int lim = l->size ();
    for (u_int i = 0; i < lim; i++) {
      if (!(fn = (*l)[i]->eval ()) || fn.len () <= 0) {
	PWARN ("Bad filename in inclist (" << ali << "," << i << ")");
	err = true;
      } else if (!(b = parser->to_binding (fn))) {
	PWARN (fn << ": cannot access file");
	err = true;
      } else {
	files.push_back (b->fn); // use completed filename
	if (!pub->do_explore ())
	  delete b;
      }
    }
  }
  ali++;
  return (!err);
}

bool
pfile_inclist_t::validate ()
{
  if (files.size () <= 0) {
    PWARN ("No files given to inclist () command");
    err = true;
  }
  return (!err);
}

bool
pfile_include2_t::add (ptr<arglist_t> l)
{
  // XXX kind of kludgey, but fn should never be set if we're in 
  // v2 of the include file.
  assert (!fn);
  ptr<pstr_t> s;
  ptr<pval_t> v;

  if (!err) {
    if (fn_v2) {
      PWARN("Include tags only take one p-argument");
      err = true;
    } else if (!add_base (l)) {
      err = true;
    } else if (!(v = (*l)[0]->to_pval ()) || !(s = v->to_pstr ())) {
      PWARN("Bad filename in include; must be a string!");
      err = true;
    } else {
      assert (s);
      fn_v2 = s;
    }
  }
  return (!err);
}

bool
pfile_include_t::add_base (ptr<arglist_t> l) 
{
  if (l->size () <= 0 || l->size () > 2) {
    PWARN("Wrong number of arguments to include");
    err = true;
  } else if (l->size () == 2 && !(env = (*l)[1]->to_aarr ())) {
    PWARN("Second argument to include must be an associative array");
    err = true;
  }
  return (!err);
}

bool
pfile_include_t::add (ptr<arglist_t> l)
{
  pbinding_t *b = NULL;
  if (!err) {
    if (fn) {
      PWARN("Include tags only take one p-argument");
      err = true;
    } else if (!add_base (l)) {
      err = true;
    } else if (!(fn = (*l)[0]->eval ()) || fn.len () <= 0) {
      PWARN("Bad filename in include");
      err = true;
    } else if (!(b = parser->to_binding (fn))) {
      PWARN(fn << ": cannot access file");
      err = true;
    } else {
      fn = b->fn; // use the completed filename
      if (!pub->do_explore ())
	delete b;
    }
  }

  return (!err);
}

bool
pfile_include_t::validate ()
{
  if (!fn) {
    PWARN("No file to include");
    err = true;
  }
  return (!err);
}

bool
pfile_include2_t::validate ()
{
  if (!fn_v2 || fn_v2->is_empty ()) {
    PWARN("No file to include");
    err = true;
  }
  return (!err);
}

bool
pfile_switch_t::add (ptr<arglist_t> l)
{
  str s;
  if (!err) {
    if (key) 
      return add_case (l);
    if (l->size () != 1 || !(s = (*l)[0]->eval ())) {
      PWARN("First argument to switch must be the switch variable");
      err = true;
    } else {
      key = New refcounted<pvar_t> (s);
    }
  }
  return (!err);
}

bool
pfile_switch_t::validate ()
{
  if (!key) {
    PWARN("No key for switch statement");
    err = true;
  } else if (_all_cases.size () == 0 && !def) {
    PWARN("No cases for switch");
    err = true;
  }
  return !err;
}

bool
pfile_set_func_t::add (ptr<arglist_t> l)
{
  if (err)
    return false;
  if (aarr) {
    PWARN("Set command only takes one argument");
    err = true;
  } else if (l->size () != 1 || !(aarr = (*l)[0]->to_aarr ())) {
    PWARN("Bad argument to set command");
    err = true;
  }
  return (!err);
}

bool
pfile_switch_t::add_case (ptr<arglist_t> l)
{
  ptr<aarr_arg_t> env;
  phashp_t ph;
  str ckey;
  ptr<pub_regex_t> rxx;
  ptr<pub_range_t> range;

  // fn1 is what's provided in the given file; fn2 is
  // the output from the the to_binding call to get the jailed filename
  str fn1, fn2; 

  // a nested environment
  ptr<nested_env_t> ne;

  if (!err) {
    if (l->size () == 1) {
      if ((*l)[0]->is_null ()) {
	if (nulldef) {
	  PWARN ("More than 1 NULL case in switch statement");
	  err = true;
	} else {
	  nulldef = true;
	  return (true);
	}
      } else {
	PWARN ("Invalid case statement with no key.");
	err = true;
      }
    } else if (l->size () == 3) {
      if (!(env = (*l)[2]->to_aarr ())) {
	PWARN("Third argument in switch case must be an associative array");
	err = true;
      }
    } else if (l->size () != 2) {
      PWARN ("Wrong number of arguments in switch case");
      err = true;
    }
  }

  if (l->size () > 1 && (ne = (*l)[1]->to_nested_env ())) {
    if (l->size () != 2) {
      PWARN ("When using a nested argument with '{{ ... }}', only 2 args "
	     "expected per case");
      err = true;
    }
  } else if (l->size () > 1 && !(*l)[1]->is_null () && 
	     !(fn1 = (*l)[1]->eval ())) {
    PWARN ("Bad filename in case declaration");
    err = true;
  }

  //
  // treat zero-length file names as NULL file names.
  //
  if (fn1 && fn1.len () == 0) 
    fn1 = NULL;
  
  pbinding_t *b = NULL;
  if (!err) {
    if (!(*l)[0]->is_null () && 
	!(ckey = (*l)[0]->eval ()) &&
	!(rxx = (*l)[0]->to_regex ()) && 
	!(range = (*l)[0]->to_range ())) {
      PWARN("Cannot determine case key");
      err = true;
    } else if (fn1 && !(b = parser->to_binding (fn1))) {
      PWARN(fn1 << ": cannot access file");
      err = true;
    } else if (b) {
      fn2 = b->fn;
      if (!pub->do_explore ())
	delete b;
    }
  }

  // Here are some example cases:
  //
  //   ( 1, "/foo.html", { "a" => 10 } )
  //       ckey = "1", fn2 = "/foo.html" and the AARR is as above;
  //
  //   ( , "/xxx.html" )
  //       ckey = NULL, meaning, this is the default case, and will match
  //       anything.
  //
  //   (* , "/default.html" )
  //       ckey = *, and will be turned into ckey = NULL, 
  //       meaning, this is the default case, and will match
  //       anything that doesn't match something else.
  //
  //   (NULL, "/key-null.html" )
  //       ckey = "NULL", meaning, the key had a null value.
  //
  //   (1, )
  //       ckey = "1", if so, don't include anything. in this case,
  //       fn2 == NULL !!
  //
  if (!err) {
    if (ckey && ckey == "*") ckey = NULL;
    bool nullkey = (ckey && ckey == PUB_NULL_CASE) ;
    ptr<pswitch_env_base_t> allcase;
						      
    if (rxx || range) {
      ptr<pswitch_env_base_t> se;
      if (rxx) {
	se = New refcounted<pswitch_env_rxx_t> (rxx, fn2, env, ne);
      } else if (range) {
	se = New refcounted<pswitch_env_range_t> (range, fn2, env, ne);
	warn << "range in effect!\n";
      }
      if (se) {
	_other_cases.push_back (se);
	allcase = se;
      }

    } else if ((!ckey && def) || (ckey && _exact_cases[ckey]) || 
	       (nullkey && nullcase)) {
      PWARN("Doubly-defined case statement in switch");
      err = true;
    } else {
      if (nullkey) ckey = NULL;

      if (!ckey) {

	ptr<pswitch_env_nullkey_t> nke = 
	  New refcounted<pswitch_env_nullkey_t> (fn2, env, ne);

	if (nullkey)
	  nullcase = nke;
	else 
	  def = nke;

      } else {
	ptr<pswitch_env_exact_t> se = 
	  New refcounted<pswitch_env_exact_t> (ckey, fn2, env, ne);
	_exact_cases.insert (ckey, se);
	allcase = se;
      }
    }

    if (!err && allcase)
      _all_cases.push_back (allcase);

    // might have a NULL file if none was requested
    if (!err && fn2)
      files.push_back (fn2);
    
  }
  return (!err);
}

bool
pfile_g_include_t::add (ptr<arglist_t> l)
{
  if (err)
    return false;
  if (l->size () < 1) {
    PWARN("Wrong number of arguments to include");
    err = true;
  } else {
    
    // true if the last element is an associate array
    bool last_aa = (*l)[l->size () - 1]->to_aarr ();

    if ((last_aa && l->size () == 4) || (!last_aa && l->size () == 3)) {
      if (!(pubobj = (*l)[0]->eval ()) || pubobj.len () <= 0) {
	PWARN("Bad first argument to include; pub object expected");
	err = true;
      } else {
	l->pop_front ();
      }
    } else {

      //
      // no first arg was given (which corresponds to the name of the 
      // publishing object); thus, we'll just assume the default name.
      //
      pubobj = str (ok_pubobjname);
    }

    if (!err) {
      bool ret = pfile_g_ctinclude_t::add (l);
      if (ret)
	PFILE->add_ifile (fn);
      return ret;
    }
  }
  return (!err);
}

bool
pfile_g_ctinclude_t::add (ptr<arglist_t> l)
{
  if (err) 
    return false;
  if (l->size () != 3 && l->size () != 2) {
    PWARN("Bad syntax in ct_include arguments");
    err = true;
  } else if (!(osink = (*l)[0]->eval ()) || osink.len () <= 0) {
    PWARN("Invalid output sink given to ct_include function");
    err = true;
  } else if (!(fn = (*l)[1]->eval ()) || fn.len () <= 0) {
    PWARN("No filename given for ct_include");
    err = true;
  } else if (l->size () == 3 && !(env = (*l)[2]->to_aarr ())) {
    PWARN("Bad third argument given to ct_include; shoud be ass-arr");
    err = true;
  } else if (ct_read () && !parser->to_binding (fn)) {
    PWARN(fn << ": cannot open file");
    err = true;
  }
  return (!err);
}

void
pfile_switch_t::explore (pub_exploremode_t mode) const
{
  str fn;
  for (u_int i = 0; i < files.size (); i++) {
    fn = files[i];
    if (fn) ::explore (mode, fn);
  }
  if (def && (fn = def->filename ()))
    ::explore (mode, fn);

  if (nullcase && (fn = nullcase->filename ()))
    ::explore (mode, fn);
}

void
output_std_t::output_file_loc (penv_t *e, int lineno)
{
  if (lineno == 0)
    lineno++;
  pfile_type_t m = switch_mode (PFILE_TYPE_CODE);
  str fn = e->filename ();
  if (fn)
    output (e, strbuf ("\n#line ") << lineno << " \"" << fn << "\"" << "\n");
  switch_mode (m);
}

void
output_std_t::output_raw (penv_t *e, const str &s)
{
  if (!_muzzled)
    out->cat (s);
}

void
output_std_t::output (penv_t *e, zbuf *zb, bool quoted)
{
  if (_muzzled)
    return;

  switch (mode) {
  case PFILE_TYPE_H:
  case PFILE_TYPE_WH:
  case PFILE_TYPE_CODE:
    out->cat (*zb);
    break;
  case PFILE_TYPE_WEC:
  case PFILE_TYPE_EC:
  case PFILE_TYPE_GUY:
    {
      vec<zstr> zs;
      zb->to_zstr_vec (&zs);
      u_int len = zs.size ();
      for (u_int i = 0; i < len; i++) {
	output (e, zs[i], quoted);
      }
    }
    break;
  case PFILE_TYPE_CONF:
  default:
    break;
  }
}

void
output_std_t::output (penv_t *e, const zstr &s, bool quoted)
{
  if (_muzzled)
    return;

  switch (mode) {
  case PFILE_TYPE_H:
  case PFILE_TYPE_WH:
  case PFILE_TYPE_CODE:
    *out << s;
    break;
  case PFILE_TYPE_WEC:
  case PFILE_TYPE_EC:
  case PFILE_TYPE_GUY: 
    {
      vec<str> ss;
      assert (osink);
      if (str_split (&ss, s, quoted)) {
	for (u_int i = 0; i < ss.size (); i++) {
	  if (osink_open) {
	    if (i == 0) *out << "     << " << ss[i] << "\n";
	    else        *out << "        " << ss[i] << "\n";
	  } else {
	    *out << "  " << osink << " << " << ss[i] << "\n";
	    osink_open = true;
	  }
	}
      } else {
	output_err (e, "str_split failed");
      }
    }
    break;
  case PFILE_TYPE_CONF:
    break;
  default:
    break;
  }
}

void
output_t::output_err_stacktrace (penv_t *e, const str &s, int l)
{
  str stk = e->stack_to_str ();
  strbuf b = s << "\n" << stk;
  output_err (e, b, l);
}

str
penv_t::stack_to_str () const
{
  strbuf b;
  int sz = fstack.size ();
  if (file)
    b << "\t#" << sz-- << " " << file->loc (0) ;
  for (; sz >= 0; sz--) {
    b << "\n"; // leave the last \n off!
    b << "\t#" << sz << " " << fstack[sz]->loc ();
  }
  return b;
}

bool
penv_t::i_stack_add (bpfcp_t t)
{
  if (istack[t->bnd->hash ()])
    return false;
  istack.insert (t->bnd->hash ());
  push_file (t);
  return true;
}

void
penv_t::i_stack_remove (bpfcp_t t)
{
  pop_file ();
  istack.remove (t->bnd->hash ());
}

void
output_std_t::output_header (penv_t *e)
{
  pfile_type_t om = mode;
  if (mode == PFILE_TYPE_EC) 
    mode = PFILE_TYPE_CODE;

  if (mode == PFILE_TYPE_CODE) {
    str fn = e->filename ();
    output (e, strbuf () << "\n/*\n * Generated by pub from file: " << fn
	    << "\n * DO NOT EDIT!!!\n" 
	    << " */\n\n" );
    e->needloc = true;
  }
  mode = om;
}

void
output_std_t::output_info (penv_t *e, const str &s, int l)
{
  if (!e->output_info ())
    return;
  switch (mode) {
  case PFILE_TYPE_EC:
  case PFILE_TYPE_WEC:
  case PFILE_TYPE_H:
  case PFILE_TYPE_WH:
  case PFILE_TYPE_GUY:
    output (e, strbuf () << "<!-- " << e->loc (l) << ": " << s << " -->");
    break;
  case PFILE_TYPE_CODE:
    output (e, strbuf () << "/* " << e->loc (l) << ": " << s << " */\n");
    break;
  default:
    break;
  }
}

void
output_std_t::output_err (penv_t *e, const str &s, int l)
{
  switch (mode) {
  case PFILE_TYPE_H:
  case PFILE_TYPE_WH: 
    {
      strbuf m ("Output error (");
      m << e->loc (l) << "): " << s;
      str msg = m;
      if (_opts & P_VISERR) {
	output (e, strbuf () << "<font color=red><b>" << m 
		<< "</b></font><br>\n");
      } else {
	output (e, strbuf () << "<!-- " << msg << " --->");
      }
      warn << m << "\n";
      break;
    }
  case PFILE_TYPE_GUY:
  case PFILE_TYPE_CODE:
  case PFILE_TYPE_EC:
    e->compile_err (s);
    break;
  default:
    warn << "Uncaught error (" << e->loc (l) << "): " << s << "\n";
    exit (1);
  }
}

void
output_conf_t::output_err (penv_t *e, const str &s, int l)
{
  warn << "Conf file eval error (" << e->loc (l) << "): " << s << "\n";
}

void
pfile_set_func_t::output (output_t *o, penv_t *e) const
{
  o->output_set_func (e, this);
}

void
output_std_t::output_set_func (penv_t *e, const pfile_set_func_t *s)
{
  s->output_runtime (e);
}

void
output_conf_t::output_set_func (penv_t *e, const pfile_set_func_t *s)
{
  s->output_config (e);
}

void
pfile_g_ctinclude_t::output (output_t *o, penv_t *e) const
{
  e->needloc = true;
  if (!osink) {
    o->output_err (e, "no output sink given", lineno);
    return;
  }
  o->output_file_loc (e, lineno);
  
  pfile_type_t m = o->switch_mode (PFILE_TYPE_GUY);
  str oo = o->switch_osink (osink);

  pfile_include_t::output (o, e);

  o->switch_osink (oo);
  o->switch_mode (m);

}

aarr_t &
aarr_t::add (const str &n, zbuf *z)
{
  add (New nvpair_t (n, New refcounted<pval_zbuf_t> (z))) ;
  return (*this);
}

aarr_t &
aarr_t::add (const str &n, ptr<zbuf> zb)
{
  add (New nvpair_t (n, New refcounted<pval_zbuf_t> (zb))) ;
  return (*this);
}

aarr_t &
aarr_t::add (const str &n, const str &v)
{
  add (New nvpair_t (n, New refcounted<pstr_t> (v)));
  return (*this);
}

void
pfile_g_init_pdl_t::output (output_t *o, penv_t *e) const
{
  e->needloc = true;
  vec<pfnm_t> v = file->get_ifiles ();
  strbuf b ("\n");
  u_int lim = v.size ();
  for (u_int i = 0; i < lim; i++) {
    if (i != 0) b << "\n";
    b << "\tadd_pubfile (\"" << v[i] << "\");";
  }
  pfile_type_t m = o->switch_mode (PFILE_TYPE_CODE);
  o->output (e, b);
  o->switch_mode (m);
}

void
pfile_g_include_t::output (output_t *o, penv_t *e) const
{
  e->needloc = true;
  if (!pubobj) {
    o->output_err (e, "no pub object given", lineno);
    return;
  }

  if (!fn) {
    o->output_err (e, "no file to include", lineno);
    return;
  }

  if (!osink) {
    o->output_err (e, "no suio to output to", lineno);
    return;
  }

  o->output_file_loc (e, lineno);

  pfile_type_t m = o->switch_mode (PFILE_TYPE_CODE);

  str arg4;
  if (env) {
    strbuf a ("_pv");
    a << e->aarr_n++;
    strbuf b ("   {\n\taarr_t ");
    b << a << ";\n\t" << a;
    o->output (e, b);
    env->output (o, e);
    arg4 = strbuf ("&") << a;
  } else {
    arg4 = "NULL";
  }
  str flags = (e->opts & P_DEBUG) ? "P_DEBUG" : "0";
  o->output (e, strbuf ("\t") << pubobj << "->include (&" << osink << ", " 
	     << c_escape (fn) << ", " << flags << ", "  
	     << arg4 << ");\n");
  if (env) 
    o->output (e, "\t}\n");

  o->switch_mode (m);
}

static void
_output_aarr (output_t *o, penv_t *e, int *i, const nvpair_t &n)
{
  n.output (o, e, *i);
  (*i)++;
}

void
aarr_t::output (output_t *o, penv_t *e) const
{
  int i = 0;
  aar.traverse (wrap (_output_aarr, o, e, &i));
  o->output (e, "\t  ;\n");
}

void
nvpair_t::output (output_t *o, penv_t *e, int i) const
{
  if (!val)
    return;
  str sv = val->eval (e, EVAL_COMPILE);
  if (!sv)
    return;

  strbuf b;
  if (i > 0)
    b << "\t    ";
  b << ".add (";
  o->output (e, b);
  o->output (e, c_escape (nm), true);
  o->output (e, strbuf (", ") << sv << ")\n");
}

void
pbuf_t::output (output_t *o, penv_t *e) const
{
  for (pbuf_el_t *p = els.first; p; p = els.next (p))
    p->output (o, e);
}

str
pbuf_t::to_str (pub_evalmode_t m, bool allownull) const 
{
  strbuf b;
  str s;
  if (els.size () == 1) 
    return els.first->to_str_2 (m);
  if (m == EVAL_COMPILE)
    b << "strbuf ()";
  pbuf_el_t *e = els.first;

  if (!e && allownull)
    return NULL;

  for ( ; e; e = els.next (e))
    if ((s = e->to_str_2 (m))) {
      if (m == EVAL_COMPILE)
	b << " << ";
      b.cat (s, true); // have to copy, since s will go out of scope
    }
  return b;
}

void
pbuf_t::add (pbuf_el_t *v)
{
  n++;
  els.insert_tail (v);
}

void
pbuf_t::add (const str &s)
{
  if (!els.last || !els.last->concat (s)) {
    n++;
    els.insert_tail (New pbuf_str_t (s));
  }
}

str
pbuf_str_t::to_str_2 (pub_evalmode_t m) const
{
  switch (m) {
  case EVAL_COMPILE:
    return c_escape (to_str ());
  default:
    return to_str ();
  }
}

str
pbuf_zbuf_t::to_str_2 (pub_evalmode_t m) const
{
  switch (m) {
  case EVAL_COMPILE:
    return c_escape (zb->output ()); 
  default:
    return zb->output ();
  }
}

str 
pbuf_var_t::to_str_2 (pub_evalmode_t m) const
{
  switch (m) {
  case EVAL_COMPILE:
    return val;
  default:
    return NULL;
  }
}

void
pfile_gprint_t::output (output_t *o, penv_t *e) const
{
  e->needloc = true;

  if (!outv)
    return;

  o->output_file_loc (e, lineno);

  pfile_type_t m = o->switch_mode (PFILE_TYPE_GUY);
  str oo = o->switch_osink (outv);

  pfile_html_sec_t::output (o, e);

  o->switch_osink (oo);
  o->switch_mode (m);
}

pfile_type_t
output_std_t::switch_mode (pfile_type_t m)
{
  pfile_type_t r = mode;
  bool cmode = (mode == PFILE_TYPE_GUY || mode == PFILE_TYPE_EC ||
		mode == PFILE_TYPE_WEC);
  if (cmode && osink_open) {
      *out << "    ;\n";
      osink_open = false;
  }
  if (cmode)
    osink_open = false;
  mode = m;
  return r;
}

str
output_std_t::switch_osink (const str &no)
{
  str oo = osink;
  osink = no;
  return oo;
}

str
output_std_t::switch_class (const str &nc)
{
  str oc = classname;
  classname = nc;
  return oc;
}

void
pfile_t::add_section2 (pfile_sec_t *s) 
{
  if (!s)
    s = pop_section ();
  if (!s) 
    return;
  else if (s->is_empty ())
    delete s;
  else if (section)
    section->add (s, false);
  else
    secs.insert_tail (s);
}

void
pfile_t::add_section (pfile_sec_t *s)
{
  if (!s) 
    s = pop_section ();
  if (s)
    if (s->is_empty ())
      delete s;
    else
      secs.insert_tail (s);
}

// ------------------------------------------------------------------------
// Dump routines
//   - debugging information dump
// ------------------------------------------------------------------------


void
dumper_t::begin_obj (const str &s, void *p, int l)
{
  level ++;
  set_sp_buf ();
  str sp = str (sp_buf, level * DUMP_INDENT - 2);
  warnx ("%s-{ %s (%p", sp.cstr (), s.cstr (), p);
  if (l)
    warnx << "," << l;
  warnx << ")\n";
}

void
dumper_t::end_obj ()
{
  warnx << str (sp_buf, level * DUMP_INDENT - 1) << "}\n";
  level--;
}

void
dumper_t::output (const str &s, bool nl)
{
  warnx << " " << sp_buf << s;
  if (nl) warnx << "\n";
}

void
dumper_t::set_sp_buf ()
{
  memset (sp_buf, ' ', MAXLEV);
  sp_buf[level*DUMP_INDENT] = '\0';
}

static void
_gvars_t_dump (dumper_t *d, const str &s)
{
  DUMP(d, "gv: " << s);
}

void
gvars_t::dump2 (dumper_t *d) const
{
  gvtab_t *p = const_cast<gvtab_t *> (&tab);
  p->traverse (wrap (_gvars_t_dump, d));
}

void
pstr_t::dump2 (dumper_t *d) const 
{
  DUMP(d, "num elements: " << n);
  els.dump (d);
}

void
pval_zbuf_t::dump2 (dumper_t *d) const
{
  DUMP(d, zb->output ());
}

void
pfile_include_t::dump2 (dumper_t *d) const
{
  if (fn) 
    DUMP (d, "fn: " << fn);
  env->dump (d);
}

void
pfile_include2_t::dump2 (dumper_t *d) const
{
  if (fn_v2) {
    DUMP (d, "fn_v2: ");
    fn_v2->dump2 (d);
  }
  env->dump (d) ;
}


void
pfile_inclist_t::dump2 (dumper_t *d) const
{
  u_int lim = files.size ();
  for (u_int i = 0; i < lim; i++)
    DUMP (d, "fn: " << files[i]);
}


void
pfile_g_ctinclude_t::dump2 (dumper_t *d) const 
{
  str os = osink; if (!os) os = "(NULL)";
  DUMP(d, "osink: " << os <<  "; fn: " << fn);
}

void
pfile_g_include_t::dump2 (dumper_t *d) const 
{
  str os = osink; if (!os) os = "(NULL)";
  str po = pubobj; if (!po) po = "(NULL)";
  DUMP(d, "osink: " << os << "; fn: " << fn);
  DUMP(d, "pubobj: " << po);
  env->dump (d);
}

void
pfile_html_el_t::dump2 (dumper_t *d) const
{
  str c = to_str ();
  DUMP(d, "-{ data (" << c.len () << "):");
  d->output (c, false);
  DUMP(d, " }");
}

void
pfile_gprint_t::dump2 (dumper_t *d) const 
{
  DUMP(d, "outv: " << outv);
  pfile_html_sec_t::dump2 (d);
}

void
pfile_code_t::dump2 (dumper_t *d) const 
{
  str s = sb;
  DUMP(d, "-{ code (" << s.len () << "):");
  d->output (s, false);
  DUMP(d, " }");
  pfile_sec_t::dump2 (d);
}

void
bound_pfile_t::dump2 (dumper_t *d) const
{
  DUMP(d, "filename: " << filename () );
  file->dump (d);
}

void
pfile_t::dump2 (dumper_t *d) const 
{
  str t;
  switch (pft) {
  case PFILE_TYPE_NONE:
    t = "none";
    break;
  case PFILE_TYPE_GUY:
    t = "guy";
    break;
  case PFILE_TYPE_H:
    t = "html";
    break;
  case PFILE_TYPE_WH:
    t = "ws-stripped html";
    break;
  case PFILE_TYPE_CODE:
    t = "c++-code";
    break;
  case PFILE_TYPE_WEC:
    t = "ws-stripped Embedded C++";
    break;
  case PFILE_TYPE_EC:
    t = "Embedded C++";
    break;
  default:
    t = "other/bad file type";
    break;
  }
  DUMP(d, " type: " << t);
  secs.dump (d);
}

void
pvar_t::dump2 (dumper_t *d) const
{
  DUMP(d, "varname: " << nm);
}

void
pswitch_env_base_t::dump2 (dumper_t *d) const
{
  str k = get_key ();
  DUMP(d, "key: " << (k? k: str ("NONE")) 
       << "; fn: " << (fn ? fn : str ("NONE")));
  if (aarr) aarr->dump (d);
}

void
pfile_switch_t::dump2 (dumper_t *d) const
{
  if (key)
    key->dump (d);
  if (def) {
    DUMP(d, "default case:");
    def->dump (d);
  }
  DUMP(d, "other cases:");
  
  for (size_t i = 0; i < _all_cases.size (); i++) {
      _all_cases[i]->dump (d);
    }
	 }

void
nvpair_t::dump2 (dumper_t *d) const
{
  DUMP(d, "name: " << nm);
  val->dump (d);
}

static void
_aarr_t_dump (dumper_t *d, nvpair_t *n)
{
  n->dump (d);
}

void
aarr_t::dump2 (dumper_t *d) const 
{
  nvtab_t *t = const_cast<nvtab_t *> (&aar);
  t->traverse (wrap (_aarr_t_dump, d));
}

void
dumpable_t::dump (dumper_t *d) const
{
  d->begin_obj (get_obj_name (), (void *)this, get_lineno ());
  dump2 (d);
  d->end_obj ();
}

bool
penv_t::is_gvar (const str &v) const
{
  int rc;
  for (int i = gvars.size () - 1; i >= 0; i--) 
    if ((rc = gvars[i]->lookup (v)))
      return (rc < 0 ? false : true);
  return false;
}

void
pfile_code_t::output (output_t *o, penv_t *e) const
{
  pfile_type_t m = o->switch_mode (PFILE_TYPE_CODE);
  if (e->needloc) {
    o->output_file_loc (e, lineno);
    e->needloc = false;
  }
  o->output (e, sb);
  o->switch_mode (m);
}

void
pfile_sec_t::explore (pub_exploremode_t mode) const
{
  if (els)
    for (pfile_el_t *e = els->first; e; e = els->next (e))
      e->explore (mode);
}

void
pfile_t::explore (pub_exploremode_t mode) const
{
  for (pfile_sec_t *s = secs.first; s; s = secs.next (s))
    s->explore (mode);
}

void
parr_mixed_t::dump2 (dumper_t *d) const
{
  DUMP (d, "n elements: " << v.size ());
  u_int lim = v.size ();
  for (u_int i = 0; i < lim; i++)
    v[i]->dump (d);
}

aarr_t &
aarr_t::overwrite_with (const aarr_t &a)
{
  aar.overwrite_with (a.aar);
  return (*this);
}

penv_state_t *
penv_t::start_output (aarr_t *a, u_int o)
{
  penv_state_t *r = New penv_state_t (getopts (), size (), cerrflag);
  opts = o;
  if (o & P_GLOBALSET) {
    _global_set = New refcounted<aarr_t> ();
    safe_push (_global_set);
  }
  if (a) push (a);
  cerrflag = false;
  return r;
}

bool
penv_t::finish_output (penv_state_t *s)
{
  bool ret = success ();
  opts = s->opts;
  resize (s->estack_size);
  cerrflag = s->errflag;
  delete s;
  return ret;
}

void
pfile_set_func_t::output_config (penv_t *e) const
{
  env = e;
  if (aarr) env->safe_push (aarr);
}

xpub_status_typ_t
pub_stat2status (pubstat_t in)
{
  switch (in) {
  case PUBSTAT_OK:
    return  (XPUB_STATUS_OK);
    break;
  case PUBSTAT_FNF:
    return (XPUB_STATUS_NOENT);
    break;
  case PUBSTAT_READ_ERROR:
  case PUBSTAT_PARSE_ERROR:
    return (XPUB_STATUS_ERR);
    break;
  }
  return XPUB_STATUS_ERR;
}

void
nvtab_t::copy (const nvtab_t &in)
{
  deleteall ();
  overwrite_with (in);
}

void
nvtab_t::overwrite_with (const nvtab_t &in)
{
  for (const nvpair_t *p = in.first (); p; p = in.next (p)) {
    insert (New nvpair_t (*p));
  }
}

void
pbuf_t::eval_obj (pbuf_t *b, penv_t *e, u_int m) const
{
  b->add (New pbuf_connector_t (mkref (const_cast<pbuf_t *> (this))));
}

//-----------------------------------------------------------------------
//
// Code for flattening data objects, as needed when pub2 clients 
// load in configuration variables and resolve them.
//
ptr<pval_t>
pval_t::flatten (penv_t *e) 
{
  return eval_to_pbuf (e, EVAL_FULL);
}

ptr<pval_t>
pint_t::flatten (penv_t *e) 
{
  return mkref (this); 
}

ptr<pval_t>
parr_mixed_t::flatten (penv_t *e) 
{
  ptr<parr_mixed_t> n = New refcounted<parr_mixed_t> ();
  for (size_t s = 0; s < v.size (); s++) {
    n->add (v[s]->flatten (e));
  }
  return n;
}

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
//

penv_t::penv_t (aarr_t *a, u_int o, aarr_t *g)
  : aarr_n (1), file (NULL), needloc (false), opts (o), evm (EVAL_FULL),
    olineno (-1), cerrflag (false), tlf (true)
{ 
  if (g) push (g);
  if (a) push (a); 
}

penv_t::penv_t (const penv_t &e)
  : aarr_n (e.aarr_n), file (e.file), needloc (e.needloc),
    cerr (e.cerr), opts (e.opts), evm (e.evm),
    estack (e.estack), gvars (e.gvars), fstack (e.fstack), hold (e.hold),
    istack (e.istack), olineno (e.olineno), 
    _localizer (e._localizer) {}

//
//-----------------------------------------------------------------------

void
pfile_set_local_func_t::output_runtime (penv_t *e) const
{
  push_frame (e, aarr);
}

void
pfile_set_func_t::output_runtime (penv_t *e) const
{
  // set_global() will return false if global-setting was not enabled
  // for this particular output run.  In that case, revert to the old
  // behavior, as seen in pfile_set_local_func (by pushing on a frame).
  // This case comes up when using pub2::output_conf2_t class, which
  // does its own flattening.
  if (aarr && !e->set_global (*aarr))
    push_frame (e, aarr);
}

bool
penv_t::set_global (const aarr_t &a)
{
  bool ret = false;
  if (_global_set) {
    _global_set->overwrite_with (a);
    ret = true;
  }
  return ret;
}

bool
output_t::set_muzzle (bool b)
{
  bool r = _muzzled;
  _muzzled = b;
  return r;
}

aarr_t &
aarr_t::operator= (const aarr_t &in)
{
  aar.copy (in.aar);
  return (*this);
}

bool
pub_regex_t::compile (str *s)
{
  rrxx x;
  bool ret = true;
  if (!x.compile (_rxx_str.cstr (), _opts.cstr ())) {
    strbuf b;
    b << "error compiling regex: " << x.geterr ();
    *s = b;
    ret = false;
  } else {
    _rxx = New refcounted<rxx> (x);
  }
  return ret;
}

bool
pub_regex_t::match (const str &s)
{
  return _rxx && s && _rxx->match (s);
}
 
bool
pswitch_env_exact_t::match (const scalar_obj_t &so) const
{
  const str &s = so.to_str_n ();
  return s && s == _key;
}

bool
pswitch_env_rxx_t::match (const scalar_obj_t &so) const
{
  const str &s = so.to_str_n ();
  return _rxx->match (s);
}

bool
pswitch_env_range_t::match (const scalar_obj_t &so) const
{
  return _range && _range->match (so);
}
