// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */


#ifndef _LIBPUB_PUB3FUNC_H_
#define _LIBPUB_PUB3FUNC_H_

#include "pub.h"
#include "parr.h"
#include "pub3expr.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class for_t : public pfile_func_t {
  public:
    for_t (int l) : pfile_func_t (l) {}
    for_t (const xpub_for_t &x);
    bool to_xdr (xpub_obj_t *x) const;
    bool add (ptr<arglist_t> l);
    bool add_env (ptr<nested_env_t> e) { _env = e; return true; }
    str get_obj_name () const { return "pub3::for_t"; }
    virtual void publish (pub2_iface_t *, output_t *, penv_t *, 
			  xpub_status_cb_t , CLOSURE) const;
    bool publish_nonblock (pub2_iface_t *, output_t *, penv_t *) const;
    void output (output_t *o, penv_t *e) const;
  protected:
  private:
    str _iter;
    str _arr;
    ptr<nested_env_t> _env;
  };
  
  //-----------------------------------------------------------------------

  class cond_clause_t {
  public:
    cond_clause_t (int l) : _lineno (l) {}

    static ptr<cond_clause_t> alloc (int l) 
    { return New refcounted<cond_clause_t> (l); }


    void add_expr (ptr<expr_t> e) { _expr = e; }
    void add_env (ptr<nested_env_t> e) { _env = e; }

  private:
    int _lineno;
    ptr<expr_t> _expr;
    ptr<nested_env_t> _env;
  };

  //-----------------------------------------------------------------------

  typedef vec<ptr<cond_clause_t> > cond_clause_list_t;

  //-----------------------------------------------------------------------
  
  class cond_t : public pfile_func_t {
  public:
    cond_t (int l) : pfile_func_t (l) {}
    void add_clauses (ptr<cond_clause_list_t> c) { _clauses = c; }
    str get_obj_name () const { return "pub3::cond_t"; }
    bool to_xdr (xpub_obj_t *x) const { return false; }
    void publish (pub2_iface_t *, output_t *, penv_t *, 
		  xpub_status_cb_t , CLOSURE) const;
    bool publish_nonblock (pub2_iface_t *, output_t *, penv_t *) const;
    void output (output_t *o, penv_t *e) const;
    bool add (ptr<arglist_t> al) { return false; }
  private:
    ptr<cond_clause_list_t> _clauses;
  };

  //-----------------------------------------------------------------------
};

#endif /* _LIBPUB_PUB3FUNC_H_ */
