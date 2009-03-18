// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */


#ifndef _LIBPUB_PUB3DATA_H_
#define _LIBPUB_PUB3DATA_H_

#include "pub.h"
#include "parr.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class expr_t {
  public:

    virtual bool eval_as_bool () const = 0;
    virtual int64_t eval_as_int () const = 0;
    virtual str eval_as_str () const = 0;
    virtual scalar_obj_t eval_as_scalar () const = 0;
    virtual bool is_null () const = 0;

  };
  
  //-----------------------------------------------------------------------

  class expr_logical_t : public expr_t {
  public:
    int64_t eval_as_int () const { return eval_as_bool (); }
  };

  //-----------------------------------------------------------------------

  class expr_OR_t : public expr_logical_t {
  public:
    bool eval_as_bool () const;
    ptr<expr_t> _t1, _t2;
  };

  //-----------------------------------------------------------------------

  class expr_AND_t : public expr_logical_t  {
  public:
    bool eval_as_bool () const;
    ptr<expr_t> _f1, _f2;
  };

  //-----------------------------------------------------------------------

  class expr_NOT_t : public expr_logical_t  {
  public:
    bool eval_as_bool () const;
    ptr<expr_t> _e;
  };

  //-----------------------------------------------------------------------

  class expr_ordering_t : public expr_logical_t {
  public:
    expr_ordering_t (bool lt, bool eq);
    bool eval_as_bool () const;

    bool _lt, _eq;
    ptr<expr_t> _l, _r;
  };

  //-----------------------------------------------------------------------

  class expr_EQ_t : public expr_logical_t {
  public:
    expr_EQ_t (bool neg) : _neg (neg) {}
    bool eval_as_bool () const;

    bool _neg;
    ptr<expr_t> _o1, _o2;
  };

  //-----------------------------------------------------------------------

};

#endif /* _LIBPUB_PUB3EXPR_H_ */

