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
#include "pub3.h"
#include "parr.h"

int
pval_w_t::to_int () const
{
  if (ival_flag) 
    return ival;
  const pval_t *v = get_pval ();
  if (!v)
    return int_err;

  int64_t t;

  // will work on pub v3 objects, too!
  if (!v->to_int64 (&t)) 
    return int_err;

  if (t > INT_MAX || t < INT_MIN)
    return int_err;
  return int (t);
}

str
pval_w_t::to_str () const 
{
  ptr<pvar_t> pv;
  ptr<const pub3::expr_t> x;

  const pval_t *v = get_pval ();

  if (v && (x = v->to_expr ())) {
    return x->to_str ();
  }

  if (name) pv = New refcounted<pvar_t> (name) ;
  else if (val) pv =  New refcounted<pvar_t> (val);
  else if (ival_flag) return strbuf () << ival;
  else return NULL;
  return pv->eval (env, EVAL_INTERNAL);
}

const pval_t *
pval_w_t::get_pval () const
{
  const pval_t *v = val;
  if (!v && name && env) {
    v = env->lookup (name, false);
  }
  return v;
}

pval_w_t 
pval_w_t::elem_at (size_t i) const
{
  pval_t *v = const_cast<pval_t *> (get_pval ());
  ptr<pub3::expr_list_t> x;
  pval_t *out = NULL;

  if (v) {

    if ((x = v->to_expr_list ()) && (i < x->size ())) {
      out = (*x)[i];
    }
    
    if (!out) {
      const parr_mixed_t *m = v->to_mixed_arr ();
      if (m && i < m->size ()) {
	out = (*m)[i];
      }
    }
    
    if (!out) {
      const parr_ival_t *ia = v->to_int_arr ();
      if (ia) {
	int t;
	if (ia->val (i, &t) == PARR_OK) {
	  return pval_w_t (t);
	}
      }
    }
  }
  
  if (out) {
    return pval_w_t (out, env);
  }

  return pval_w_t ();
}

//-----------------------------------------------------------------------

size_t
pval_w_t::size () const
{
  const pval_t *v = get_pval ();
  if (!v) return 0;
  ptr<const pub3::expr_list_t> x;

  if ((x = v->to_expr_list ())) { return x->size (); }

  const parr_t *a = v->to_arr ();
  return (a ? a->size () : 0);
}

//-----------------------------------------------------------------------

pval_w_t &
pval_w_t::operator= (const pval_w_t &in)
{
  val = in.val;
  name = in.name;
  int_err = in.int_err;
  env = in.env;
  ival_flag = in.ival_flag;
  ival = in.ival;
  return (*this);
}

//-----------------------------------------------------------------------


