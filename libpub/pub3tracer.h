// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#pragma once

#include "pub3.h"
#include "pub3lib.h"

namespace pub3 {

    class pub_tracer_t;

    // Returns a nullptr if it fails...
    ptr<pub_tracer_t> start_trace(obj_dict_t *m_obj,
                                  str file,
                                  const bhash<str> &pure_funs);
    ptr<expr_t> end_trace(ptr<const pub_tracer_t>, const str &out);

    ptr<expr_t> locobj(const eval_t &p);

    //--------------------------------------------------------------------------
    // ! Argument evaluation
    //--------------------------------------------------------------------------
    // The pub interpreter is somewhat lazy and doesn't always give you fully
    // interpreted values
    // e.g: `f(g(x))`
    // passes an expression representing `g(x)` as argument to `f` instead of
    // evaluating `g(x)` and passing the result to `f` (i.e.: it is up to the
    // callee to evaluate its arguments).

    // Publib.t::eval_args
    callable_t::args_t eval_args(eval_t *e, const callable_t::args_t &in);

    void
    pub_args(eval_t *p, const callable_t::args_t &in, expr_list_t *, evv_t,
             CLOSURE);
    //--------------------------------------------------------------------------
    // ! Native function replacement
    //--------------------------------------------------------------------------

    // A serial replacement for rfn3::shotgun
    // This is used to ensure deterministic playback
    class serial_shotgun_t : public patterned_fn_blocking_t {
     public:
        serial_shotgun_t() : patterned_fn_blocking_t("rfn3", "shotgun", "l") {}
        void v_pub_to_val_2(eval_t *, const vec<arg_t> &, cxev_t,
                            CLOSURE) const;
    };

    //--------------------------------------------------------------------------
    // Seriously: the compilers needs this but you don't...

    struct pub_tracer_t {
        ptr<expr_dict_t> globals;
        ptr<expr_dict_t> universals;
        ptr<expr_list_t> trace = expr_list_t::alloc();
        ptr<expr_dict_t> libraries;
        str file;
        bool failed = false;
    };

}  // namespace tracefun

