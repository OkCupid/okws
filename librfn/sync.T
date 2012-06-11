// -*-c++-*-

#include "okrfn-int.h"
#include "okformat.h"
#include "pub3out.h"

namespace rfn3 {
#if 0
}
#endif


//-----------------------------------------------------------------------

tamed static void
fork_hlp (ptr<publish_t> pub, ptr<const callable_t> fn)
{
  tvars {
    ptr<const expr_t> x;
    ptr<expr_list_t> args;
  }
  
  // The function passed should be a lambda that doesn't take an arguments,
  // so pass in an empty list here.
  args = expr_list_t::alloc ();
  pub = pub->clone ();
  
  twait { fn->pub_to_val (pub, args, mkevent (x)); }
}

//-----------------------------------------------------------------------

ptr<const expr_t>
fork_t::v_eval_2 (publish_t *pub, const vec<arg_t> &args) const
{
  
  //
  // MK 2011/6/7
  //
  // We need to make a wholesale copy of the publishing environment, so that we
  // and the other thread of computation can independently change it.  I think
  // this should cover us, but I'm thruthfully a little unsure about it.
  // Note, this change supercedes the early change of making publish_t's 
  // inherit from refcounted, since it's no longer needed...
  //
  fork_hlp (pub->clone (), args[0]._F);

  return expr_null_t::alloc ();
}

//-----------------------------------------------------------------------

const str fork_t::DOCUMENTATION =
  "returns immediately and executes a blocking function in the background";

//-----------------------------------------------------------------------

tamed void
sleep_t::v_pub_to_val_2 (eval_t *e, const checked_args_t &args, cxev_t ev) const
{
  tvars {
    time_t s;
    time_t ms (0);
    time_t ns;
  }
  s = args[0]._i;
  if (args.size () == 2) { ms = args[1]._i; }
  ns = ms * 1000 * 1000;
  
  twait { delaycb (s, ns, mkevent ()); }
  ev->trigger (expr_null_t::alloc ());
}

//-----------------------------------------------------------------------

const str sleep_t::DOCUMENTATION = "sleeps for s seconds and ms milliseconds";

//-----------------------------------------------------------------------

tamed void
shotgun_t::v_pub_to_val_2 (eval_t *e, const checked_args_t &args, cxev_t ev) 
  const
{
  tvars {
    vec<ptr<const callable_t> > v;
    ptr<expr_list_t> l;
    bool ok (true);
    size_t i;
    ptr<expr_t> out;
    vec<ptr<const expr_t> > results;
    ptr<expr_list_t> list_out;
    ptr<expr_list_t> fnargs;
  }
  

  l = args[0]._l;

  for (i = 0; ok && i < l->size (); i++) {
    ptr<const callable_t> f = (*l)[i]->to_callable ();
    if (!f) {
      strbuf b ("expected a list of lambdas, but arg %zu is not of proper type",
		i+1);
      report_error (e, b);
      ok = false;
    } else {
      v.push_back (f);
    }
  }

  if (ok) {
    results.setsize (v.size ());

    // The function passed should be a lambda that doesn't take an arguments,
    fnargs = expr_list_t::alloc ();

    twait {
      for (i = 0; i < v.size (); i++) {
	v[i]->pub_to_val (e, fnargs, mkevent (results[i]));
      }
    }

    // If any of the callees fail, then give them a proper null, not a 
    // C++ null value.
    list_out = expr_list_t::alloc ();
    list_out->setsize (v.size ());
    for (i = 0; i < v.size (); i++) {
      if (results[i]) {
	(*list_out)[i] = results[i]->copy ();
      } else {
	(*list_out)[i] = expr_null_t::alloc ();
      }
    }
    out = list_out;
  }
  ev->trigger (out);
}

//-----------------------------------------------------------------------

const str shotgun_t::DOCUMENTATION =
  "executes a list of blocking functions in parallel, returns when they've all "
  "completed";

};
