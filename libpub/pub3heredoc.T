// -*-c++-*-

#include "pub3.h"
#include "pub3out.h"
#include "pub3heredoc.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  heredoc_t::heredoc_t (ptr<zone_t> z, lineno_t l)
    : expr_t (l), _body (z){}

  //-----------------------------------------------------------------------

  ptr<heredoc_t> heredoc_t::alloc (ptr<zone_t> z)
  { return New refcounted<heredoc_t> (z, plineno ()); }

  //-----------------------------------------------------------------------

  tamed void
  heredoc_t::pub_to_val (eval_t *p, cxev_t ev) const
  {
    tvars {
      ptr<output_t> old_output;
      ptr<output_t> new_output;
      zbuf z;
      ptr<expr_t> out;
      strbuf b;
      xpub_status_t xs;
    }

    if (_body) {
      new_output = New refcounted<output_std_t> (&z);
      old_output = p->set_output (new_output);

      // Propogate the WSS state from the outer to the inner
      // publishing environment
      new_output->enable_wss (old_output->get_wss_enabled ());

      twait { _body->publish (p, mkevent (xs)); }
      if (xs.status == XPUB_STATUS_OK) {
	z.to_strbuf (&b, false);
	out = expr_str_t::alloc (b);
      } 
      p->set_output (old_output);
    }

    if (!out) {
      out = expr_null_t::alloc ();
    }

    ev->trigger (out);
  }

  //-----------------------------------------------------------------------

  void
  heredoc_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "body:", _body);
  }

  //-----------------------------------------------------------------------

};
