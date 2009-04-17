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
#include "pub3.h"

//-----------------------------------------------------------------------

class oksrvc_configtest_t : public oksrvc_t {
public:
  oksrvc_configtest_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
};

//-----------------------------------------------------------------------

class okclnt_configtest_t : public okclnt_t {
public:
  okclnt_configtest_t (ptr<ahttpcon> x, oksrvc_configtest_t *o)
    : okclnt_t (x, o), ok_configtest (o) {}
  ~okclnt_configtest_t () {}

  void process () { process_T (); }
  void process_T (CLOSURE); 

protected:
  void run_global (evv_t ev, CLOSURE);
  void run_local (evv_t ev, CLOSURE);
  oksrvc_configtest_t *ok_configtest;
};

//-----------------------------------------------------------------------

bool
str2ref (str in, str *n, vec<size_t> *out)
{
  vec<str> v;
  static rxx dot ("\\.");
  split (&v, dot, in);
  bool ret = true ;
  if (v.size () > 0) {
    *n = v.pop_front ();
    for (size_t i = 0; ret && i < v.size (); i++) {
      size_t tmp;
      if (!convertint (v[i], &tmp)) {
	warn << "Cannot convert to int: " << v[i] << "\n";
	ret = false;
      }
      out->push_back (tmp);
    }
  } else {
    ret = false;
  }
  return ret;

}

//-----------------------------------------------------------------------

tamed void 
okclnt_configtest_t::process_T ()
{
  if (cgi.exists ("global")) {
    twait { run_global (mkevent ()); }
  } else {
    twait { run_local (mkevent ()); }
  }
  output (out);
}

//-----------------------------------------------------------------------

tamed void
okclnt_configtest_t::run_local (evv_t ev)
{
  tvars {
    aarr_t dct;
    bool rc;
  }

  twait { pub2 ()->run_cfg (cgi["fn"], mkevent (rc), &dct); }
  if (!rc) {
    out << "Error including file " << cgi["fn"] << " or file wasn't given\n";
  } else {
    str lst = cgi["display"];
    vec<str> vars;
    static rxx comma (",");
    split (&vars, comma, lst);
    for (size_t i = 0; i < vars.size (); i++) {
      vec<size_t> indices;
      str key;
      if (str2ref (vars[i], &key, &indices)) {
	ptr<pval_t> v = dct.lookup_ptr (key);
	ptr<const parr_mixed_t> m;
	const parr_ival_t *iv;
	ptr<pub3::expr_list_t> el;
	str s;

	for (size_t j = 0; v && j < indices.size (); j++) {
	  size_t indx = indices[j];
	  if ((m = v->to_mixed_arr ()) && indx < m->size ()) {
	    v = (*m)[indx];
	  } else if ((iv = v->to_int_arr ()) && indx < iv->size ()) {
	    v = NULL;
	    int64_t x;
	    if (iv->val (indx, &x)) {
	      s = strbuf () << x;
	    }
	  } else if ((el = v->to_expr_list ())) {
	    v = (*el)[indx];
	  }
	}

	if (v) {
	  ptr<pub3::expr_t> x;
	  ptr<pub_scalar_t> ps;
	  pbuf_t *pb;
	  if ((x = v->to_expr ())) {
	    s = x->to_str ();
	  } else if ((ps = v->to_pub_scalar ())) {
	    s = ps->obj ().to_str ();
	  } else if ((pb = dynamic_cast<pbuf_t *>(static_cast<pval_t *> (v)))) {
	    s = pb->to_str ();
	  } else {
	    s = v->eval_simple ();
	  }
	}
	if (!s) s = "(null)";
	out << vars[i] << " -> " << s << "\n";
      }
    }
  }
  ev->trigger ();


}


//-----------------------------------------------------------------------

tamed void
okclnt_configtest_t::run_global (evv_t ev)
{
  tvars {
    bool rc (true);
  }

  twait { pub2 ()->run_cfg (cgi["fn"], mkevent (rc)); }
  if (!rc) {
    out << "Error including file " << cgi["fn"] << " or file wasn't given\n";
  } else {
    str lst = cgi["display"];
    vec<str> vars;
    static rxx comma (",");
    split (&vars, comma, lst);
    for (size_t i = 0; i < vars.size (); i++) {
      vec<size_t> indices;
      str key;
      if (str2ref (vars[i], &key, &indices)) {
	pval_w_t tmp = (*pub2 ())[key];
	for (size_t j = 0; j < indices.size (); j++) {
	  pval_w_t tmp2 = tmp[indices[j]];
	  tmp = tmp2;
	}
	str s = tmp.to_str ();
	if (!s) s = "(null)";
	out << vars[i] << " -> " << s << "\n";
      }
    }
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

oksrvc_t::newclnt_t *
oksrvc_configtest_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_configtest_t (x, this);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_configtest_t (argc, argv);
  oksrvc->launch ();
  amain ();
}

//-----------------------------------------------------------------------
