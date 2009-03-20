// -*-c++-*-
/* $Id: web.h 4029 2009-01-30 13:28:14Z max $ */

#ifndef _LIBRFN_OKRFN_H_
#define _LIBRFN_OKRFN_H_

#include "pub3.h"
#include "qhash.h"

namespace rfn1 {

  using namespace pub3;

  //-----------------------------------------------------------------------

  typedef ptr<runtime_fn_t> (*constructor_t) 
  (const str &n, ptr<expr_list_t> l, int line, str *err);

  //-----------------------------------------------------------------------

  class std_factory_t : public rfn_factory_t {
  public:

    std_factory_t ();

    ptr<runtime_fn_t>
    alloc (const str &s, ptr<expr_list_t> l, int lineno);

    qhash<str, constructor_t> _tab;

  };

  //-----------------------------------------------------------------------

  class random_t : public runtime_fn_t {
  public:
    
    random_t (const str &n, ptr<expr_list_t> el, int lineno, 
	      ptr<expr_t> l, ptr<expr_t> h);

    static ptr<runtime_fn_t> 
    constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err);

  private:
    ptr<expr_t> _low;
    ptr<expr_t> _high;
  };

  //-----------------------------------------------------------------------


};

#endif /* _LIBRFN_OKRFN_H_ */
