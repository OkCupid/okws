// -*-c++-*-

#include "okrfn-int.h"
#include "okws_rxx.h"
#include "crypt.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  enumerate_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> out = expr_list_t::alloc ();
    ptr<const expr_list_t> in = args[0]._l;
    size_t len = in ? in->size () : size_t (0);
    for (size_t i = 0; i < len; i++) {
      ptr<expr_list_t> tuple = expr_list_t::alloc ();
      tuple->push_back (expr_uint_t::alloc (i));
      tuple->push_back (safe_copy ((*in)[i]));
      out->push_back (tuple);
    }
    return out;
  }

  //-----------------------------------------------------------------------

  const str enumerate_t::DOCUMENTATION =
    "Given a list //l//, output a list of lists, "
    "in which the //i//th element is the pair [//i//, //l//[//i//] ]";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  randsel_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> out = expr_list_t::alloc ();
    ptr<const expr_list_t> in = args[0]._l;
    size_t i = rnd.getword () % in->size ();
    out->push_back ((*in)[i]->copy ());
    out->push_back (expr_int_t::alloc (i));
    return out;
  }

  //-----------------------------------------------------------------------

  const str randsel_t::DOCUMENTATION =
    "Select a random element from the list //l//.  Return "
    "a tuple, with the element in the 0th slot, and the index of the element "
    "in the 1st slot.  Don't modify the list.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  decorate_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> out = expr_list_t::alloc ();
    ptr<const expr_list_t> in = args[0]._l;
    size_t len = in ? in->size () : size_t (0);
    for (size_t i = 0; i < len; i++) {
      ptr<const expr_t> x = (*in)[i];
      ptr<const expr_dict_t> xd;
      ptr<expr_t> el;
      if (!x) { el = expr_null_t::alloc (); }
      else if ((xd = x->to_dict ())) {
	ptr<expr_dict_t> nxd = xd->copy_dict ();
	nxd->insert ("iter", expr_int_t::alloc (i));
	nxd->insert ("odd", expr_bool_t::alloc (i % 2 != 0));
	nxd->insert ("even", expr_bool_t::alloc (i % 2 == 0));
	nxd->insert ("count", expr_int_t::alloc (len));
	nxd->insert ("first", expr_bool_t::alloc (i == 0));
	nxd->insert ("last", expr_bool_t::alloc (len - 1 == i));
	el = nxd;
      }
      out->push_back (el);
    }
    return out;
  }

  //-----------------------------------------------------------------------

  const str decorate_t::DOCUMENTATION =
    "Given a list of dictionaries //l//, annotate each "
    "row in the dictionary with a //first//, //last//, //iter//, //even// "
    "and //odd// field.  This is mainly a holdover from a previous OKWS.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  range_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    int64_t s = args.size () >= 3 ? args[2]._i : 1;
    int64_t l = args.size () >= 2 ? args[0]._i : 0;
    int64_t h = args.size () >= 2 ? args[1]._i : args[0]._i;

    ptr<expr_list_t> el = New refcounted<expr_list_t> ();
    for (int64_t i = l; i < h; i += s) {
      el->push_back (expr_int_t::alloc (i));
    }
    return el;
  }

  //-----------------------------------------------------------------------

  const str range_t::DOCUMENTATION =
    "If called with one argument (//a1//), output a integers "
    "in [0,//a1//). If called with two arguments (//a1// and //a2//), output "
    "a list of integers in [//a1//, //a2//). If called with three arguments, "
    "//a3// is the step between elements.";

  //-----------------------------------------------------------------------


  ptr<const expr_t>
  join_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    strbuf b;
    vec<str> hold;
    ptr<const expr_list_t> l = args[1]._l;
    str js = args[0]._s;

    for (size_t i = 0; i < l->size (); i++) {
      if (i > 0) {
	b << js;
      }

      str s;
      ptr<const expr_t> x = (*l)[i];

      if (!x || x->is_null ()) { s = ""; }
      else s = x->to_str (false);
      b << s;
      hold.push_back (s);
    }
    return expr_str_t::safe_alloc (b);
  }

  //-----------------------------------------------------------------------

  const str join_t::DOCUMENTATION =
    "Take the list of strings //l// and join them together"
    "with the delimiter //delim//.  Output a string.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  reverse_t::v_eval_1 (eval_t *p, const margs_t &args) const
  {
    ptr<expr_t> x;
    ptr<expr_list_t> l;
    ptr<const expr_t> cel;
    ptr<expr_t> el;

    if (args.size () != 1) {
      report_error (p, "reverse() takes 1 argument");
    } else if (!(x = args[0])) {
      report_error (p, "argument to reverse() must be non-null");
    } else if (!(l = x->to_list ())) {
      report_error (p, "argument to reverse() must be a list");
    } else {
      l->reverse ();
    }
    return l;
  }

  //-----------------------------------------------------------------------

  const str reverse_t::DOCUMENTATION =
    "Reverse the input list //in place//.  Return the reversed list.";

  //-----------------------------------------------------------------------

  static void
  shuffle (expr_list_t &v)
  {
    for (ssize_t i = v.size () - 1; i >= 0; i--) {
      ssize_t j = rnd.getword () % (i + 1);
      if (j != i) {
	ptr<pub3::expr_t> x = v[i];
	v[i] = v[j];
	v[j] = x;
      }
    }
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  shuffle_t::v_eval_1 (eval_t *p, const margs_t &args) const
  {
    ptr<expr_t> x;
    ptr<expr_list_t> l;
    ptr<const expr_t> cel;
    ptr<expr_t> el;

    if (args.size () != 1) {
      report_error (p, "shuffle() takes 1 argument");
    } else if (!(x = args[0])) {
      report_error (p, "argument to shuffle() must be non-null");
    } else if (!(l = x->to_list ())) {
      report_error (p, "argument to shuffle() must be a list");
    } else {
      shuffle (*l);
    }
    return l;
  }

  //-----------------------------------------------------------------------

  const str shuffle_t::DOCUMENTATION =
    "Shuffle the input list //in place//.  Return the shuffled list.";

  //-----------------------------------------------------------------------

  static void
  safe_push (ptr<expr_list_t> out, ptr<const expr_t> cel)
  {
    ptr<expr_t> el;
    if (cel) el = cel->copy ();
    if (!el) el = expr_null_t::alloc ();
    out->push_back (el);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  splice_t::v_eval_1 (eval_t *p, const margs_t &args) const
  {
    ptr<expr_t> x;
    ptr<expr_list_t> l;
    ptr<const expr_t> cel;
    ptr<expr_t> el;
    ptr<expr_list_t> out;
    int64_t i, r;

    if (args.size () < 3) {
      report_error (p, "splice() takes 3 or more arguments");
    } else if (!(x = args[0])) {
      report_error (p, "first argument to splice() must be non-null");
    } else if (!(l = x->to_list ())) {
      report_error (p, "first argument to splice() must be a list");
    } else if (!args[1]->to_int (&i)) {
      report_error (p, "second argument to splice() must be an int");
    } else if (!args[2]->to_int (&r)) {
      report_error (p, "third argument to splice() must be an int");
    } else {
      out = expr_list_t::alloc ();
      size_t lim = min<size_t> (l->size(), i);
      for (size_t j = 0; j < lim; j++)           { safe_push (out, (*l)[j]); }
      for (size_t j = 3; j < args.size (); j++)  { safe_push (out, args[j]); }
      for (size_t j = i + r; j < l->size(); j++) { safe_push (out, (*l)[j]); }
    }
    return out;
  }

  //-----------------------------------------------------------------------

  const str splice_t::DOCUMENTATION =
    "Like JS's splice() function. Remove //remove// objects "
    "from the list //v// starting at index //index//.  Replace with optional "
    "objects //o_1//, through //o_n//, as many as specified.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  slice_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    auto in = args[0]._l;
    auto out = expr_list_t::alloc();
    int start = 0, end = in->size();

    switch (args.size()) {
        // Yep, fallthrough
        case 3: end = ((args[2]._i < 0) ? in->size() : 0) + args[2]._i;
        case 2: start = ((args[1]._i < 0) ? in->size() : 0) + args[1]._i;
    }

    if (start < 0) { start = 0; }
    // If end < 0, it'll be promoted to unsigned
    if (end > 0 && end > in->size()) { end = in->size(); }

    if (start < end) { out->reserve(end - start); }

    while (start < end) {
        out->push_back((*in)[start]);
        start++;
    }

    return out;
  }

  //-----------------------------------------------------------------------

  const str slice_t::DOCUMENTATION =
    "Like JS's slice() function. Make a shallow copy of a list, "
    "optionally just part of it.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  append_t::v_eval_1 (eval_t *p, const margs_t &args) const
  {
    ptr<expr_t> x;
    ptr<expr_list_t> l;
    ptr<const expr_t> cel;
    ptr<expr_t> el;

    if (args.size () < 2) {
      report_error (p, "append() takes 2 or more arguments");
    } else if (!(x = args[0])) {
      report_error (p, "first argument to append() must be non-null");
    } else if (!(l = x->to_list ())) {
      report_error (p, "first argument to append() must be a list");
    } else {
      for (size_t i = 1; i < args.size (); i++) { safe_push (l, args[i]); }
    }
    return l;
  }
  //-----------------------------------------------------------------------

  const str append_t::DOCUMENTATION =
    "Given the list //v//, append onto it the "
    "object //o//.  There is no output; changes are made to //v// directly. ";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  reserve_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    args[0]._l->reserve(args[1]._u);
    return nullptr;
  }
  //-----------------------------------------------------------------------

  const str reserve_t::DOCUMENTATION =
    "Internally reserve space in a list to hold at least //n// items. Just a "
    "performance optimization for when you know how big it will be.";

  //-----------------------------------------------------------------------
  
  ptr<const expr_t>
  map_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_dict_t> d = args[0]._d;
    ptr<const expr_t> x = args[1]._O;

    return eval_internal (p, d, x);
  }

  //-----------------------------------------------------------------------

  const str map_t::DOCUMENTATION =
    "Map every element in //o// to the value in the "
    "dictionary //d//, more or less. If //o// is a list, then map each "
    "element of the list via //d//.  If //o// is a dictionary, map the "
    "values of //o// through //d//.";

  //-----------------------------------------------------------------------

  ptr<expr_t>
  map_t::eval_internal (eval_t *p, ptr<const expr_dict_t> m, 
			ptr<const expr_t> x) const
  {
    ptr<const expr_list_t> l;
    ptr<const expr_dict_t> cd;
    str s;
    ptr<expr_t> ret;

    if ((l = x->to_list ())) {
      ptr<expr_list_t> out = New refcounted<expr_list_t> ();
      for (size_t i = 0; i < l->size (); i++) {
	ptr<const expr_t> cx = (*l)[i];
	ptr<const expr_t> cn = eval_internal (p, m, cx);
	ptr<expr_t> n;
	if (cn) n = cn->copy ();
	if (!n) n = expr_null_t::alloc ();
	out->push_back (n);
      }
      ret = out;
    } else if ((cd = x->to_dict ())) {
      bindtab_t::const_iterator_t it (*cd);
      ptr<expr_dict_t> d = expr_dict_t::alloc ();
      const str *k;
      ptr<expr_t> cx;
      while ((k = it.next (&cx))) {
	ptr<const expr_t> cn = eval_internal (p, m, cx);
	ptr<expr_t> n;
	if (cn) n = cn->copy ();
	if (!n) n = expr_null_t::alloc ();
	d->insert (*k, n);
      }
      ret = d;
    } else if ((s = x->to_str (false))) {
      ptr<const expr_t> cx = m->lookup (s);
      if (cx) { ret = cx->copy (); }
    } else if (x->is_null ()) {
      /* noop */
    } else {
      report_error (p, "second argument to map() must be a vec, "
		    "dict or string");
    }
    if (!ret) { ret = expr_null_t::alloc (); }
    return ret;
  }

  //-----------------------------------------------------------------------

  static ptr<expr_list_t>
  split_vec_to_list (const vec<str> &v)
  {
    ptr<expr_list_t> ret = New refcounted<expr_list_t> ();
    for (size_t i = 0; i < v.size (); i++) {
      ptr<expr_t> e;
      if (v[i]) e = New refcounted<expr_str_t> (v[i]);
      else e = expr_null_t::alloc ();
      ret->push_back (e);
    }
    return ret;

  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  split_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<rxx> rx = args[0]._r;
    str s = args[1]._s;
    vec<str> v;
    split (&v, *rx, s);
    return split_vec_to_list (v);
  }

  //-----------------------------------------------------------------------

  const str split_t::DOCUMENTATION =
    "Given a regex //r// and a string //s//, split //s// into "
    "a list of strings, akin to the Python function 'split'.  Return a list "
    "of strings.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  split2_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<rxx> rx = args[0]._r;
    str s = args[1]._s;
    vec<str> v;
    split2 (&v, *rx, s);
    return split_vec_to_list (v);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  contains_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_list_t> l = args[0]._l;
    ptr<const expr_t> o = args[1]._O;
    bool ret = false;
    for (size_t i = 0; !ret && i < l->size (); i++) {
      ptr<const expr_t> val = (*l)[i];
      if (val) { ret = expr_EQ_t::eval_static (val, o); }
    }
    return expr_bool_t::alloc (ret);
  }

  //-----------------------------------------------------------------------

  const str contains_t::DOCUMENTATION =
    "Return //true// if the list //l// contains the object "
    "//o// and //false// otherwise.  If you find yourself using this a lot, "
    "consider using a dictionary instead, which has faster lookup times.";

  //-----------------------------------------------------------------------

  tamed void
  pop_front_t::pub_to_mval (eval_t *p, callable_t::args_t args, 
			    xev_t ev) const
  {
    tvars {
      ptr<expr_t> x;
      ptr<expr_t> ret;
    }

    if (count_args (p, args->size ())) {
      twait { (*args)[0]->pub_to_mval (p, mkevent (x)); }
      ret = eval_final (p, x);
    }

    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  ptr<expr_t> 
  pop_front_t::eval_final (eval_t *p, ptr<expr_t> x) const
  {
    ptr<expr_t> ret;
    ptr<expr_list_t> l;
    if (!x) {
      report_error (p, "first argument to pop_front() must be non-null");
    } else if (!(l = x->to_list ())) {
      report_error (p, "first argument to pop_front() must be a list");
    } else if (l->size ()) {
      ret = l->pop_front ();
    } else {
      ret = expr_null_t::alloc ();
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  pop_front_t::pub_to_val (eval_t *p, callable_t::args_t args, 
			   cxev_t ev) const
  {
    tvars {
      ptr<expr_t> out;
    }
    twait { pub_to_mval (p, args, mkevent (out)); }
    ev->trigger (out);
  }

  //-----------------------------------------------------------------------

  tamed void
  pop_front_t::pub_to_ref (eval_t *p, callable_t::args_t args, 
			   mrev_t ev) const
  {
    tvars {
      ptr<expr_t> out;
      ptr<mref_t> r;
    }
    twait { pub_to_mval (p, args, mkevent (out)); }
    if (out) { r = const_mref_t::alloc (out); }
    ev->trigger (r);
  }

  //-----------------------------------------------------------------------

  ptr<mref_t> 
  pop_front_t::eval_to_ref (eval_t *p, args_t args) const
  {
    ptr<mref_t> ret;
    ptr<expr_t> x = eval_to_mval (p, args);
    if (x) { ret = const_mref_t::alloc (x); }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t> pop_front_t::eval_to_val (eval_t *p, args_t args) const
  { return eval_to_mval (p, args); }

  //-----------------------------------------------------------------------

  bool
  pop_front_t::count_args (eval_t *p, size_t s) const
  {
    bool ret = true;
    if (s != 1) {
      ret = false;
      report_error (p, "pop_front() takes one argument");
    }
    return ret;
  }

  //-----------------------------------------------------------------------
  
  ptr<expr_t> 
  pop_front_t::eval_to_mval (eval_t *p, args_t args) const
  {
    ptr<expr_t> x , ret;

    if (count_args (p, args->size ())) {
      x = (*args)[0]->eval_to_mval (p); 
      ret = eval_final (p, x);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  list_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_list_t> cret;
    ptr<expr_list_t> ret;
    ptr<const expr_t> o = args[0]._O;
    ptr<const expr_dict_t> dict;
    str s;

    if ((cret = o->to_list ())) { /* noop */ }
    else if ((dict = o->to_dict ())) {
      ret = expr_list_t::alloc ();
      bindtab_t::const_iterator_t it (*dict);
      const str *key;
      ptr<expr_t> x;
      while ((key = it.next (&x))) {
	ptr<expr_list_t> t = expr_list_t::alloc ();
	t->push_back (expr_str_t::alloc (*key));
	t->push_back (expr_cow_t::alloc (x));
	ret->push_back (t);
      }
      cret = ret;
    } else if ((s = o->to_str ())) {
      ret = expr_list_t::alloc ();
      for (const char *cp = s.cstr (); *cp; cp++) {
	char buf[2];
	buf[1] = 0;
	buf[0] = *cp;
	ret->push_back (expr_str_t::alloc (buf));
      }
      cret = ret;
    } else {
      str t = o->type_to_str ();
      report_error (p, strbuf("cannot convert object of type '%s' to list",
			      t.cstr ()));
      
    }
    return cret;
  }

  //-----------------------------------------------------------------------

  const str list_t::DOCUMENTATION =
    "Output object //o// as a list.  If //o// is "
    "a list to begin with, just output //o//.  If //o// is a string, "
    "output the list of characters that compose //o//. If //o// "
    "is a dictionary, act like the //items// call.";

  //-----------------------------------------------------------------------

  static ptr<const callable_t> g_cmp;
  static ptr<expr_list_t> g_target;
  static ptr<expr_list_t> g_tmp_args;
  static eval_t *g_eval;

  //-----------------------------------------------------------------------

  static int cmp_fn (const void *a, const void *b)
  {
    size_t *sa = const_cast<size_t *> (static_cast<const size_t *> (a));
    size_t *sb = const_cast<size_t *> (static_cast<const size_t *> (b));

    (*g_tmp_args)[0] = (*g_target)[*sa];
    (*g_tmp_args)[1] = (*g_target)[*sb];
    ptr<const expr_t> x = g_cmp->eval_to_val (g_eval, g_tmp_args);
    int r = x ? x->to_int () : -1;
    return r;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  sort2_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> in = args[0]._l;
    g_tmp_args = expr_list_t::alloc ();
    g_tmp_args->setsize (2);
    g_cmp = New refcounted<cmp_t> ();
    g_eval = p;

    ptr<const callable_t> keyfn;
    if (args.size () > 1) { keyfn = args[1]._F; } 

    size_t n = in->size ();

    vec<size_t> v;
    v.setsize (n);
    for (size_t i = 0; i < n; i++) { v[i] = i; }

    if (keyfn) {
      g_target = expr_list_t::alloc ();
      g_target->setsize (n);
      ptr<expr_list_t> key_args = expr_list_t::alloc ();
      key_args->setsize (1);
      for (size_t i = 0; i < n; i++) { 
	(*key_args)[0] = (*in)[i];
	ptr<const expr_t> cx = keyfn->eval_to_val (p, key_args);
	ptr<expr_t> x;
	if (!cx) { x = expr_null_t::alloc (); }
	else { x = cx->copy (); }
	(*g_target)[i] = x;
      }
    } else {
      g_target = in;
    }
    qsort (v.base (), n, sizeof (size_t), cmp_fn);
    
    ptr<expr_list_t> ret = expr_list_t::alloc ();
    ret->setsize (n);
    for (size_t i = 0; i < n; i++) {
      (*ret)[i] = (*in)[v[i]];
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  sort_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    g_target = args[0]._l;
    g_tmp_args = expr_list_t::alloc ();
    g_tmp_args->setsize (2);

    g_eval = p;

    if (args.size () > 1) { g_cmp = args[1]._F; } 
    else { g_cmp = New refcounted<cmp_t> (); }

    size_t n = g_target->size ();
    vec<size_t> v;
    v.setsize (n);
    for (size_t i = 0; i < n; i++) { v[i] = i; }
    qsort (v.base (), n, sizeof (size_t), cmp_fn);
    
    ptr<expr_list_t> ret = expr_list_t::alloc ();
    ret->setsize (n);
    for (size_t i = 0; i < n; i++) {
      (*ret)[i] = (*g_target)[v[i]];
    }

    return ret;
  }

  //-----------------------------------------------------------------------

  const str sort_t::DOCUMENTATION =
    "Sort the given list //l//, given the comparison "
    "function //cmp//.  If //cmp// isn't specified, then use the "
    "default comparison operatore.  If it is, use it!";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  cmp_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_t> a1 = args[0]._O;
    ptr<const expr_t> a2 = args[1]._O;
    
    int res = 0;
    if (expr_relation_t::eval_final (p, a1, a2, XPUB3_REL_LT, this)) {
      res = -1; 
    } else if (expr_relation_t::eval_final (p, a1, a2, XPUB3_REL_GT, this)) {
      res = 1;
    }

    return expr_int_t::alloc (res);
  }

  //-----------------------------------------------------------------------

  const str cmp_t::DOCUMENTATION =
    "The standard comparison function. Will return "
    "-1 for a < b, 1 for a > b and 0 for a == b.  Will compare strings "
    "or integers, nothing else.";

  //-----------------------------------------------------------------------

};

