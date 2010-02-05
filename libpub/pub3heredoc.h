// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub.h"
#include "pub3ast.h"
#include "pub3eval.h"

namespace pub3 {

  class heredoc_t : public expr_t {
  public:
    call_t (ptr<zone_html_t> z, lineno_t l) : expr_t (l), _body (z) {}
    call_t (const xpub3_heredoc_t &x);
    static ptr<heredoc_t> alloc (ptr<zone_html_t> z);
    static ptr<heredoc_t> alloc (const xpub3_heredoc_t &x);
    bool to_xdr (xpub3_expr_t *x) const;
    const char *get_obj_name () const { return "pub3::heredoc_t"; }

    ptr<const expr_t> eval_to_val (eval_t *e) const { return NULL; }
    ptr<mref_t> eval_to_ref (eval_t *e) const { return NULL; }
    bool might_block_uncached () const { return true; }

    void pub_to_val (publish_t *p, cxev_t ev, CLOSURE) const;
    void pub_to_ref (publish_t *p, mrev_t ev, CLOSURE) const;

    void v_dump (dumper_t *d) const;
  private:
    ptr<zone_html_t> _body;
  };
  

};
