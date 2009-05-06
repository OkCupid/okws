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
#include "crypt.h"
#include "pub3.h"

#define DISABLE_GZIP -2

//-----------------------------------------------------------------------

void
zstr_to_xdr (const zstr &z, xpub_zstr_t *x, int l)
{
  x->s = z.to_str ();
  if (l != DISABLE_GZIP)
    x->zs = z.compress (l);
  x->clev = l;
}

//-----------------------------------------------------------------------

zstr
xdr_to_zstr (const xpub_zstr_t &x)
{
  str zs (x.zs.base (), x.zs.size ()); // XXX memcpy -- ineffecient
  return zstr (x.s, zs, x.clev);
}

//-----------------------------------------------------------------------

bool
pfile_html_sec_t::to_xdr (xpub_section_t *x) const
{
  x->typ = XPUB_HTML_SEC;
  x->lineno = lineno;
  pfile_el_t *e;
  size_t i;

  if (els) {
    x->els.setsize (els->size ());
    for (i = 0, e = els->first; e; e = els->next (e))
      if (e->to_xdr (&(x->els[i])))
	  i++;
  }
  return true;
}

//-----------------------------------------------------------------------

pfile_html_sec_t::pfile_html_sec_t (const xpub_section_t &x)
  : pfile_sec_t (x.lineno)
{
  size_t lim = x.els.size ();
  for (size_t i = 0; i < lim; i++) 
    pfile_sec_t::add (pfile_el_t::alloc (x.els[i]));
}

//-----------------------------------------------------------------------

pfile_sec_t::pfile_sec_t (const xpub_section_t &x)
  : lineno (x.lineno),
    _might_block (-1)
{
  // XXX - reconsider this copy and paste
  size_t lim = x.els.size ();
  for (size_t i = 0; i < lim; i++) 
    add (pfile_el_t::alloc (x.els[i]));
}

//-----------------------------------------------------------------------

pfile_el_t *
pfile_el_t::alloc (const xpub_obj_t &x) 
{
  switch (x.typ) {
  case XPUB_HTML_EL:
    return New pfile_html_el_t (*x.html_el);
  case XPUB_INCLIST:
    return New pfile_inclist_t (*x.inclist);
  case XPUB_INCLUDE:
    return New pfile_include_t (*x.include);
  case XPUB_INCLUDE2:
    return New pfile_include2_t (*x.include);
  case XPUB_LOAD:
    return New pfile_load_t (*x.include);
  case XPUB_FILE_PSTR:
    return New pfile_pstr_t (*x.file_pstr);
  case XPUB_FILE_VAR:
    return New pfile_var_t (*x.file_var);
  case XPUB_SWITCH:
    return New pfile_switch_t (*x.swtch);
  case XPUB_SET_FUNC:
    return New pfile_set_func_t (*x.set_func);
  case XPUB_SET_LOCAL_FUNC:
    return New pfile_set_local_func_t (*x.set_func);
  case XPUB_RAW:
    return New pfile_raw_el_t (*x.raw);
  case XPUB3_FOR:
    return New pub3::for_t (*x.forloop);
  case XPUB3_COND:
    return New pub3::cond_t (*x.cond);
  case XPUB3_INCLUDE:
    return New pub3::include_t (*x.pub3_include);
  case XPUB3_LOAD:
    return New pub3::load_t (*x.pub3_include);
  case XPUB3_INLINE_VAR:
    return New pub3::inline_var_t (*x.pub3_inline_var);
  case XPUB3_SET_FUNC:
    return New pub3::set_func_t (*x.set_func);
  case XPUB_NESTED_ENV:
    return New pfile_nested_env_t (*x.nested);
  case XPUB3_PRINT:
    return New pub3::print_t (*x.print);
  case XPUB3_EXPR_STATEMENT:
    return New pub3::expr_statement_t (*x.expr_statement);
  default:
    return NULL;
  }
}

//-----------------------------------------------------------------------

bool
pfile_html_el_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB_HTML_EL);
  zstr_to_xdr (to_zstr (), &x->html_el->data, Z_BEST_COMPRESSION);
  return true;
}

//-----------------------------------------------------------------------

void 
pfile_t::to_xdr (xpub_file_t *x) const
{
  size_t i;
  pfile_sec_t *s;
  hsh->to_xdr (&x->hsh);
  x->secs.setsize (secs.size ());
  for (i = 0, s = secs.first; s; s = secs.next (s))
    if (s->to_xdr (&(x->secs[i])))
      i++;
}

//-----------------------------------------------------------------------

void
pfile_t::init_xdr_opaque () 
{
  if (!_xdr_opaque) {
    xpub_file_t x;
    to_xdr (&x);
    _xdr_opaque = xdr2str (x);
    sha1_hash (_xdr_opaque_hash.val, _xdr_opaque.cstr (), 
	       _xdr_opaque.len ());
  }
}

//-----------------------------------------------------------------------

ssize_t
pfile_t::len () const
{
  return _xdr_opaque ? ssize_t (_xdr_opaque.len ()) : ssize_t (-1);
}

//-----------------------------------------------------------------------

ssize_t 
pfile_t::get_chunk (size_t offset, char *buf, size_t capacity) const
{
  if (!_xdr_opaque || offset >= _xdr_opaque.len ())
    return -1;
  ssize_t ret = min<ssize_t> (_xdr_opaque.len () - offset, capacity);
  memcpy (buf, _xdr_opaque.cstr () + offset, ret);
  return ret;
}

//-----------------------------------------------------------------------

pfile_t::pfile_t (const xpub_file_t &x)
  : err (PUBSTAT_OK), hsh (phash_t::alloc (x.hsh)), lineno (1), 
    pft (PFILE_TYPE_H), section (NULL)
{
  size_t lim = x.secs.size ();
  for (size_t i = 0; i < lim; i++)
    add_section (New pfile_html_sec_t (x.secs[i]));
}

//-----------------------------------------------------------------------

bool
pstr_var_t::to_xdr (xpub_pstr_el_t *x) const
{
  x->set_typ (XPUB_PSTR_VAR);
  *x->var = pvar->name ();
  return true;
}

//-----------------------------------------------------------------------

bool
pstr_str_t::to_xdr (xpub_pstr_el_t *x) const
{
  x->set_typ (XPUB_PSTR_STR);
  *x->str = to_str ();
  return true;
}

//-----------------------------------------------------------------------

bool
pstr_t::to_xdr (xpub_pstr_t *x) const 
{
  pstr_el_t *e;
  size_t i;
  x->els.setsize (els.size ());
  for (i = 0, e = els.first; e; e = els.next (e)) 
    if (e->to_xdr (&(x->els[i])))
      i++;
  return true;
}

//-----------------------------------------------------------------------

bool
pstr_t::to_xdr (xpub_val_t *x) const
{
  x->set_typ (XPUB_VAL_PSTR);
  return to_xdr (x->pstr);
}

//-----------------------------------------------------------------------

bool
pfile_pstr_t::to_xdr (xpub_obj_t *x) const 
{
  x->set_typ (XPUB_FILE_PSTR);
  pstr->to_xdr (&x->file_pstr->pstr);
  return true;
}

//-----------------------------------------------------------------------

bool
pfile_switch_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB_SWITCH);
  x->swtch->key = key->name ();
  if (def) {
    x->swtch->defcase.alloc ();
    def->to_xdr (x->swtch->defcase);
  }
  if (nullcase) {
    x->swtch->nullcase.alloc ();
    nullcase->to_xdr (x->swtch->nullcase);
  }

  x->swtch->cases.setsize (_all_cases.size ());
  for (size_t i = 0; i < _all_cases.size (); i++) {
    _all_cases[i]->to_xdr (&x->swtch->cases[i]);
  }
  x->swtch->lineno = lineno;
  x->swtch->nulldef = nulldef;
  return true;
}

//-----------------------------------------------------------------------

pfile_switch_t::~pfile_switch_t ()
{
}

//-----------------------------------------------------------------------

pfile_switch_t::pfile_switch_t (const xpub_switch_t &x) :
  pfile_func_t (x.lineno), err (false), def (NULL), 
  key (New refcounted<pvar_t> (x.key)), nulldef (x.nulldef),
  nullcase (NULL)
{
  size_t lim = x.cases.size ();
  if (x.defcase) {
    def = pswitch_env_base_t::alloc(*x.defcase)->to_nullkey();
    if (def->fn)
      files.push_back (def->fn);
  }
  if (x.nullcase) {
    nullcase = pswitch_env_base_t::alloc (*x.nullcase)->to_nullkey();
    if (nullcase->fn)
      files.push_back (nullcase->fn);
  }

  for (size_t i = 0; i < lim; i++) {
    ptr<pswitch_env_base_t> e = pswitch_env_base_t::alloc (x.cases[i]);
    if (e) {
      if (e->is_exact ()) 
	_exact_cases.insert (e->get_key (), e);
      else
	_other_cases.push_back (e);
      if (e->fn)
	files.push_back (e->fn);
    }
  }
}

//-----------------------------------------------------------------------

bool
pswitch_env_exact_t::to_xdr (xpub_switch_env_union_t *u) const
{
  u->set_typ (XPUB_SWITCH_ENV_EXACT);
  xpub_switch_env_exact_t *x = u->exact;
  x->key = _key;
  return to_xdr_base (&x->base);
}

//-----------------------------------------------------------------------

bool
pswitch_env_nullkey_t::to_xdr (xpub_switch_env_union_t *u) const
{
  u->set_typ (XPUB_SWITCH_ENV_NULLKEY);
  xpub_switch_env_nullkey_t *x = u->nullkey;
  return to_xdr_base (&x->base);
}

//-----------------------------------------------------------------------

bool
pswitch_env_rxx_t::to_xdr (xpub_switch_env_union_t *u) const
{
  u->set_typ (XPUB_SWITCH_ENV_RXX);
  xpub_switch_env_rxx_t *x = u->rxx;
  if (_rxx) {
    _rxx->to_xdr (&x->rxx);
  }
  return to_xdr_base (&x->base);
}

//-----------------------------------------------------------------------

bool
pswitch_env_range_t::to_xdr (xpub_switch_env_union_t *u) const
{
  u->set_typ (XPUB_SWITCH_ENV_RANGE);
  xpub_switch_env_range_t *x = u->range;
  if (_range) {
    _range->to_xdr (&x->range);
  }
  return to_xdr_base (&x->base);
}

//-----------------------------------------------------------------------

bool
pswitch_env_base_t::to_xdr_base (xpub_switch_env_base_t *x) const
{
  if (aarr)
    aarr->aarr_t::to_xdr (&x->aarr);

  if (fn) {
    x->body.set_typ (XPUB_SWITCH_FILE);
    *x->body.fn = fn;
  } else if (_nested_env) {
    x->body.set_typ (XPUB_SWITCH_NESTED_ENV);
    _nested_env->sec ()->to_xdr (x->body.sec);
  } else {
    x->body.set_typ (XPUB_SWITCH_NONE);
  }
  return true;
}

//-----------------------------------------------------------------------

pswitch_env_exact_t::pswitch_env_exact_t (const xpub_switch_env_exact_t &x)
  : pswitch_env_base_t (x.base),
    _key (x.key) {}

//-----------------------------------------------------------------------

pswitch_env_nullkey_t::pswitch_env_nullkey_t 
(const xpub_switch_env_nullkey_t &x)
  : pswitch_env_base_t (x.base) {}

//-----------------------------------------------------------------------

pswitch_env_rxx_t::pswitch_env_rxx_t (const xpub_switch_env_rxx_t &x)
  : pswitch_env_base_t (x.base),
    _rxx (New refcounted<pub_regex_t> (x.rxx))
{
 
  str s;
  if (!_rxx->compile (&s)) {
    warn << s << "\n";
  }
}

//-----------------------------------------------------------------------

pswitch_env_range_t::pswitch_env_range_t (const xpub_switch_env_range_t &x)
  : pswitch_env_base_t (x.base),
    _range (pub_range_t::alloc (x.range))
{}

//-----------------------------------------------------------------------

pswitch_env_base_t::pswitch_env_base_t (const xpub_switch_env_base_t &x)
  : fn (x.body.typ == XPUB_SWITCH_FILE ? str (*x.body.fn) : sNULL), 
    aarr (New refcounted<aarr_arg_t> (x.aarr)),
    _nested_env (x.body.typ == XPUB_SWITCH_NESTED_ENV ?
		 nested_env_t::alloc (*x.body.sec) : NULL) {}

//-----------------------------------------------------------------------

ptr<pswitch_env_base_t>
pswitch_env_base_t::alloc (const xpub_switch_env_union_t &x)
{
  ptr<pswitch_env_base_t> ret;
  switch (x.typ) {
  case XPUB_SWITCH_ENV_EXACT:
    ret = New refcounted<pswitch_env_exact_t> (*x.exact);
    break;
  case XPUB_SWITCH_ENV_RXX:
    ret = New refcounted<pswitch_env_rxx_t> (*x.rxx);
    break;
  case XPUB_SWITCH_ENV_NULLKEY:
    ret = New refcounted<pswitch_env_nullkey_t> (*x.nullkey);
    break;
  case XPUB_SWITCH_ENV_RANGE:
    ret = New refcounted<pswitch_env_range_t> (*x.range);
    break;
  default:
    warn << "Unexpected type of switch environment\n";
    break;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
pub_regex_t::to_xdr (xpub_regex_t *x) const
{
  x->rxx = _rxx_str;
  x->opts = _opts;
  return true;
}

//-----------------------------------------------------------------------

pub_regex_t::pub_regex_t (const xpub_regex_t &x) 
  : _rxx_str (x.rxx), _opts (x.opts) {}

//-----------------------------------------------------------------------

pub_irange_t::pub_irange_t (const xpub_irange_t &x)
  : _low (x.low), _hi (x.hi) {}

//-----------------------------------------------------------------------

ptr<nested_env_t>
nested_env_t::alloc (const xpub_section_t &s)
{
  return New refcounted<nested_env_t> (New pfile_html_sec_t (s));
}
//-----------------------------------------------------------------------

ptr<nested_env_t>
nested_env_t::alloc (const xpub_section_t *s)
{
  ptr<nested_env_t> ret;
  if (s) { 
    ret = New refcounted<nested_env_t> (New pfile_html_sec_t (*s)); 
  }
  return ret;
}

//-----------------------------------------------------------------------

pfile_nested_env_t::pfile_nested_env_t (const xpub_section_t &s)
  : _env (nested_env_t::alloc (s)) {}

//-----------------------------------------------------------------------

bool
pfile_nested_env_t::to_xdr (xpub_obj_t *o) const
{
  o->set_typ (XPUB_NESTED_ENV);
  if (_env) {
    _env->to_xdr (o->nested);
  }
  return true;
}

//-----------------------------------------------------------------------

static void
nvpair_to_xdr (xpub_aarr_t *x, size_t *i, const nvpair_t &n) 
{
  if (n.to_xdr (&x->tab[*i]))
    (*i)++;
}

//-----------------------------------------------------------------------

bool
nvpair_t::to_xdr (xpub_nvpair_t *x) const
{
  if (!val->to_xdr (&x->val))
    return false;
  x->key = nm;
  return true;
}

//-----------------------------------------------------------------------

nvpair_t::nvpair_t (const xpub_nvpair_t &x) 
  : nm (x.key), val (pval_t::alloc (x.val)) {}

//-----------------------------------------------------------------------

ptr<pval_t>
pval_t::alloc (const xpub_val_t &x) 
{
  switch (x.typ) {
  case XPUB_VAL_INT:
    return New refcounted<pint_t> (*x.i);
  case XPUB_VAL_NULL:
    return New refcounted<pval_null_t> ();
  case XPUB_VAL_PSTR:
    return New refcounted<pstr_t> (*x.pstr);
  case XPUB_VAL_MARR:
    return New refcounted<parr_mixed_t> (*x.marr);
  case XPUB_VAL_IARR:
    return parr_int_t::alloc (*x.iarr);
  case XPUB3_VAL_EXPR:
    return pub3::expr_t::alloc (**x.expr);
  default:
    return NULL;
  }
}

//-----------------------------------------------------------------------

ptr<parr_ival_t>
parr_ival_t::alloc (const xpub_parr_t &x)
{
  ptr<parr_int_t> ret;
  switch (x.typ) {
  case XPUB_CHAR:
    return New refcounted<parr_char_t> (*x.chararr);
  case XPUB_INT:
    return New refcounted<parr_int_t> (*x.intarr);
  case XPUB_INT16:
    return New refcounted<parr_int16_t> (*x.int16arr);
  case XPUB_UINT16:
    return New refcounted<parr_uint16_t> (*x.uint16arr);
  case XPUB_INT64:
    return New refcounted<parr_int64_t> (*x.hyperarr);
  case XPUB_UINT:
    return New refcounted<parr_uint_t> (*x.uintarr);
  }
  return NULL;
}

//-----------------------------------------------------------------------

parr_mixed_t::parr_mixed_t (const xpub_parr_mixed_t &x)
{
  size_t lim = x.size ();
  for (size_t i = 0; i < lim; i++)
    v.push_back (pval_t::alloc (x[i]));
}

//-----------------------------------------------------------------------

bool
parr_mixed_t::to_xdr (xpub_val_t *x) const
{
  x->set_typ (XPUB_VAL_MARR);
  size_t lim = v.size ();
  x->marr->setsize (lim);
  for (size_t i = 0; i < lim; i++) 
    v[i]->to_xdr (&((*x->marr)[i]));
  return true;
}

//-----------------------------------------------------------------------


bool
pval_null_t::to_xdr (xpub_val_t *x) const
{
  x->set_typ (XPUB_VAL_NULL);
  return true;
}

//-----------------------------------------------------------------------

bool
pint_t::to_xdr (xpub_val_t *x) const
{
  x->set_typ (XPUB_VAL_INT);
  *x->i = val;
  return true;
}

//-----------------------------------------------------------------------

bool
aarr_t::to_xdr (xpub_aarr_t *x) const
{
  size_t i = 0;
  x->tab.setsize (aar.size ());
  aar.traverse (wrap (&nvpair_to_xdr, x, &i));
  return true;
}

//-----------------------------------------------------------------------

aarr_t::aarr_t (const xpub_aarr_t &x)
{
  size_t lim = x.tab.size ();
  for (size_t i = 0; i < lim; i++) {
    add (New nvpair_t (x.tab[i]));
  }
}

//-----------------------------------------------------------------------

void
pfile_include_t::to_xdr_base (xpub_obj_t *x) const
{
  if (env)
    env->aarr_t::to_xdr (&x->include->env);
  x->include->lineno = lineno;
}

//-----------------------------------------------------------------------

bool 
pfile_include_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB_INCLUDE);
  to_xdr_base (x);
  x->include->fn.set_vrs (XPUB_V1);
  *x->include->fn.v1 = fn;
  return true;
}

//-----------------------------------------------------------------------

bool
pfile_include2_t::to_xdr_base2 (xpub_obj_t *x, xpub_obj_typ_t typ) const
{
  x->set_typ (typ);
  to_xdr_base (x);
  x->include->fn.set_vrs (XPUB_V2);
  if (fn_v2) fn_v2->to_xdr (x->include->fn.v2);
  return true;
}

//-----------------------------------------------------------------------

bool
pfile_include2_t::to_xdr (xpub_obj_t *x) const
{
  return to_xdr_base2 (x, XPUB_INCLUDE2);
}

//-----------------------------------------------------------------------

bool
pfile_load_t::to_xdr (xpub_obj_t *x) const 
{
  return to_xdr_base2 (x, XPUB_LOAD);
}

//-----------------------------------------------------------------------

bool
pfile_inclist_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB_INCLIST);
  size_t lim = files.size ();
  x->inclist->files.setsize (lim);
  for (size_t i = 0; i < lim; i++) 
    x->inclist->files[i] = files[i];
  return true;
}

//-----------------------------------------------------------------------

pfile_include_t::pfile_include_t (const xpub_include_t &x)
  : pfile_func_t (x.lineno), err (false),
    env (New refcounted<aarr_arg_t> (x.env))
{
  if (x.fn.vrs == XPUB_V1) {
    fn = *x.fn.v1;
  } else {
    warn << "Got unexpected XDR pfile representation for v1 include.\n";
    err = true;
  }
}

//-----------------------------------------------------------------------

pfile_include2_t::pfile_include2_t (const xpub_include_t &x)
  : pfile_include_t (x.lineno, New refcounted<aarr_arg_t> (x.env))
{
  if (x.fn.vrs == XPUB_V2) {
    fn_v2 = New refcounted<pstr_t> (*x.fn.v2);
  } else {
    warn << "Got unexpected XDR pfile representation for v2 include.\n";
    err = true;
  }
}

//-----------------------------------------------------------------------

pfile_load_t::pfile_load_t (const xpub_include_t &x)
  : pfile_include2_t (x) {}

//-----------------------------------------------------------------------

pfile_inclist_t::pfile_inclist_t (const xpub_inclist_t &x)
  : pfile_func_t (x.lineno), err (false)
{
  size_t lim = x.files.size ();
  for (size_t i = 0; i < lim; i++)
    files.push_back (x.files[i]);
}

//-----------------------------------------------------------------------

bool
pfile_set_func_t::to_xdr (xpub_obj_t *x) const
{
  return to_xdr_common (x, XPUB_SET_FUNC);
}

//-----------------------------------------------------------------------

bool
pfile_set_func_t::to_xdr_common (xpub_obj_t *x, xpub_obj_typ_t typ) const 
{ 
  assert (typ == XPUB_SET_FUNC || 
	  typ == XPUB_SET_LOCAL_FUNC ||
	  typ == XPUB3_SET_FUNC);
  x->set_typ (typ);
  aarr->aarr_t::to_xdr (&x->set_func->aarr);
  x->set_func->lineno = lineno;
  return true;
}

//-----------------------------------------------------------------------

bool
pfile_set_local_func_t::to_xdr (xpub_obj_t *x) const
{ 
  return to_xdr_common (x, XPUB_SET_LOCAL_FUNC);
}

//-----------------------------------------------------------------------

pfile_set_func_t::pfile_set_func_t (const xpub_set_func_t &x)
  : pfile_func_t (x.lineno), err (false),
    aarr (New refcounted<aarr_arg_t> (x.aarr)),
    env (NULL) {}

//-----------------------------------------------------------------------

bool
pfile_var_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB_FILE_VAR);
  x->file_var->lineno = lineno;
  x->file_var->var = var->name ();
  return true;
}

//-----------------------------------------------------------------------

pfile_var_t::pfile_var_t (const xpub_file_var_t &x)
  : var (New refcounted<pvar_t> (x.var)), lineno (x.lineno) {}

//-----------------------------------------------------------------------

pstr_t::pstr_t (const xpub_pstr_t &x) : n (0)
{
  size_t lim = x.els.size ();
  for (size_t i = 0; i < lim; i++)
    add (pstr_el_t::alloc (x.els[i]));
}

//-----------------------------------------------------------------------

pstr_el_t *
pstr_el_t::alloc (const xpub_pstr_el_t &x)
{
  switch (x.typ) {
  case XPUB_PSTR_VAR:
    return New pstr_var_t (*x.var);
  case XPUB_PSTR_STR:
    return New pstr_str_t (*x.str);
  case XPUB3_PSTR_EL:
    return New pub3::pstr_el_t (*x.p3el);
  default:
    return NULL;
  }
}

//-----------------------------------------------------------------------

bool
parr_int_t::to_xdr (xpub_parr_t *x) const
{
  x->set_typ (XPUB_INT);
  return parr_ival_tmplt_t<int>::to_xdr (x->intarr.addr ());
}

//-----------------------------------------------------------------------

bool
parr_char_t::to_xdr (xpub_parr_t *x) const
{
  x->set_typ (XPUB_CHAR);
  size_t lim = v.size ();
  x->chararr->setsize (lim);
  for (size_t i = 0; i < lim; i++) {
    (*x->chararr)[i] = v[i];
  }
  return true;
}

//-----------------------------------------------------------------------

bool
parr_int16_t::to_xdr (xpub_parr_t *x) const
{
  x->set_typ (XPUB_INT16);
  return parr_ival_tmplt_t<int16_t>::to_xdr (x->int16arr.addr ());
}

//-----------------------------------------------------------------------

bool
parr_uint16_t::to_xdr (xpub_parr_t *x) const
{
  x->set_typ (XPUB_UINT16);
  return parr_ival_tmplt_t<u_int16_t>::to_xdr (x->uint16arr.addr ());
}

//-----------------------------------------------------------------------

bool
parr_uint_t::to_xdr (xpub_parr_t *x) const
{
  x->set_typ (XPUB_UINT);
  return parr_ival_tmplt_t<u_int>::to_xdr (x->uintarr.addr ());
}

//-----------------------------------------------------------------------

bool
parr_int64_t::to_xdr (xpub_parr_t *x) const
{
  x->set_typ (XPUB_INT64);
  return parr_ival_tmplt_t<int64_t>::to_xdr (x->hyperarr.addr ());
}

//-----------------------------------------------------------------------

bool
pfile_raw_el_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB_RAW);
  if (_dat) {
    x->raw->dat.setsize (_dat.len ());
    memcpy (x->raw->dat.base (), _dat.cstr (), _dat.len ());
  }
  return true;
}

//-----------------------------------------------------------------------

bool
pub_irange_t::to_xdr (xpub_range_t *out)
{
  out->set_typ (XPUB_IRANGE);
  out->ir->low = _low;
  out->ir->hi = _hi;
  return true;
}

//-----------------------------------------------------------------------

static double 
cnv_double (const x_double_t &x)
{
  return (double)x.n / (double)x.d ;
}

//-----------------------------------------------------------------------

void
cnv_double (double in, x_double_t *out)
{
  int64_t d = 100000000;
  double tmp = in * d;
  int64_t n = int64_t (tmp);
  out->d = d;
  out->n = n;
}

//-----------------------------------------------------------------------

bool
pub_drange_t::to_xdr (xpub_range_t *out)
{
  out->set_typ (XPUB_DRANGE);
  cnv_double (_low, &out->dr->low);
  cnv_double (_hi, &out->dr->hi);
  return true;
}

//-----------------------------------------------------------------------

bool
pub_urange_t::to_xdr (xpub_range_t *out)
{
  out->set_typ (XPUB_URANGE);
  out->ur->low = _low;
  out->ur->hi = _hi;
  return true;
}

//-----------------------------------------------------------------------

pub_urange_t::pub_urange_t (const xpub_urange_t &in)
  : _low (in.low), _hi (in.hi) {}

//-----------------------------------------------------------------------

pub_drange_t::pub_drange_t (const xpub_drange_t &in)
  : _low (cnv_double (in.low)), _hi (cnv_double (in.hi)) {}

//-----------------------------------------------------------------------

ptr<pub_range_t>
pub_range_t::alloc (const xpub_range_t &xpr)
{
  ptr<pub_range_t> ret;
  switch (xpr.typ) {
  case XPUB_IRANGE:
    ret = New refcounted<pub_irange_t> (*xpr.ir);
    break;
  case XPUB_DRANGE:
    ret = New refcounted<pub_drange_t> (*xpr.dr);
    break;
  case XPUB_URANGE:
    ret = New refcounted<pub_urange_t> (*xpr.ur);
    break;
  default:
    break;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
nested_env_t::to_xdr (xpub_section_t *x) const
{
  bool ret = false;
  if (_sec) {
    ret = _sec->to_xdr (x);
  }
  return ret;
}

//-----------------------------------------------------------------------
