// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#include "json_rpc.h"
#include "pub3lib.h"
#include "pub3tracer.h"

//--------------------------------------------------------------------------
// ! Native functions replacement.
//--------------------------------------------------------------------------
// Those are replacements of functions in rfn3 needed for tracing. (
// note that `shotgun` is replaced by a sequential call to make the trace)
// deterministic.
namespace {
    typedef pub3::callable_t::args_t fn_args_t;
}

#define ERROR warn << "ERROR: pub3tracer.T" \
    << "[" << __LINE__ <<  "] in " << __func__ << ": "


ptr<pub3::expr_t> pub3::locobj(const pub3::eval_t &p) {
    pub3::obj_list_t ret = p.get_loc_stack()->pub();
    return ret.obj();
}

// Publib.t::eval_args
fn_args_t pub3::eval_args(pub3::eval_t *e, const fn_args_t &in) {
    ptr<pub3::expr_list_t> res = New refcounted<pub3::expr_list_t>();
    res->reserve(in->size());
    for (size_t i = 0; i < in->size(); i++) {
        ptr<const pub3::expr_t> a = (*in)[i];
        ptr<pub3::expr_t> x = a->eval_to_mval(e);
        if (!x) {
            x = pub3::expr_null_t::alloc();
        }
        res->push_back(x);
    }
    return res;
}

namespace pub3 {

    tamed void
    pub_args(eval_t *p, const fn_args_t &in, expr_list_t *out, evv_t ev) {
        tvars {
            size_t i;
            ptr<expr_t> x;
        }
        out->reserve(in->size());
        for (i = 0; i < in->size(); i++) {
            twait { (*in)[i]->pub_to_mval (p, mkevent (x)); }
            if (!x) {
                x = expr_null_t::alloc();
            }
            out->push_back(x);
        }
        ev->trigger();
    }

    tamed void
    serial_shotgun_t::v_pub_to_val_2(eval_t *e, const checked_args_t &args,
                                     cxev_t ev)
        const {
        tvars {
            vec<ptr<const callable_t> > v;
            ptr<expr_list_t> l;
            bool ok(true);
            size_t i;
            ptr<expr_t> out;
            vec<ptr<const expr_t> > results;
            ptr<expr_list_t> list_out;
            ptr<expr_list_t> fnargs;
        }

        l = args[0]._l;

        for (i = 0; ok && i < l->size(); i++) {
            ptr<const callable_t> f = (*l)[i]->to_callable ();
            if (!f) {
                strbuf b("expected a list of lambdas, but arg %zu is not "
                         " of proper type", i+1);
                report_error(e, b);
                ok = false;
            } else {
                v.push_back(f);
            }
        }

        if (ok) {
            results.setsize(v.size());

            fnargs = expr_list_t::alloc();

            for (i = 0; i < v.size(); i++) {
                twait {
                    v[i]->pub_to_val(e, fnargs, mkevent(results[i]));
                }
            }

            // If any of the callees fail, then give them a proper null,
            // not a C++ null value.
            list_out = expr_list_t::alloc();
            list_out->setsize(v.size());
            for (i = 0; i < v.size(); i++) {
                if (results[i]) {
                    (*list_out)[i] = results[i]->copy ();
                } else {
                    (*list_out)[i] = expr_null_t::alloc ();
                }
            }
            out = list_out;
        }
        ev->trigger(out);
    }


}  // namespace pub3

namespace {

    tamed static void
    fork_hlp(ptr<pub3::publish_t> pub, ptr<const pub3::callable_t> fn) {
        tvars {
            ptr<const pub3::expr_t> x;
            ptr<pub3::expr_list_t> args;
        }
        args = pub3::expr_list_t::alloc();
        pub = pub->clone();
        twait { fn->pub_to_val(pub, args, mkevent(x)); }
    }

    class fork_t : public pub3::patterned_fn_t {
        ptr<pub3::pub_tracer_t> m_tracer;
     public:
        explicit fork_t(ptr<pub3::pub_tracer_t> tracer):
            patterned_fn_t("rfn3", "fork", "l"),
            m_tracer(tracer) {}

        ptr<const pub3::expr_t>
        v_eval_2(pub3::eval_t *p, const vec<arg_t> &args) const;
    };


    ptr<const pub3::expr_t>
    fork_t::v_eval_2(pub3::publish_t *pub, const vec<arg_t> &args) const {
        if (!m_tracer->failed) {
            ERROR << "Fork is not supported by the tracing framework.\n";
            m_tracer->failed = true;
        }
        fork_hlp(pub->clone(), args[0]._F);
        return pub3::expr_null_t::alloc ();
    }

    //--------------------------------------------------------------------------
    // ! Pickling
    //--------------------------------------------------------------------------
    // We do a dfs scan of the values when doing the pickle_sanity_check. This
    // stacks is used for error reporting.
    class pickle_stack {
        str m_node;
        const pickle_stack *m_parent;
        void cat(strbuf *buf) const {
            if (m_parent) {
                m_parent->cat(buf);
                *buf << ".";
            }
            *buf << m_node;
        }
        pickle_stack(str name, const pickle_stack *parent) :
            m_node(name), m_parent(parent) {}

     public:
        str to_str() const {
            strbuf res;
            cat(&res);
            return str(res);
        }

        explicit pickle_stack(str name) : m_node(name), m_parent(nullptr) {}

        pickle_stack operator() (str name) const {
            return pickle_stack(name, this);
        }

        pickle_stack operator() (size_t idx) const {
            return pickle_stack(str(strbuf("[%ld]", idx)), this);
        }
    };

    // Check that there are values that we can't unpickle (i.e. functions).
    bool pickle_sanity_check(const pub3::expr_t &val,
                             const pickle_stack &stack) {
        const str type = val.type_to_str();
        if (type == "null" || type == "bool" || type == "str" || type == "int"
            || type == "uint" || type == "float" || type == "regex") {
            return true;
        } else if (type == "dict") {
            auto &d = *(val.to_dict());
            for (auto &key : d) {
                if (!pickle_sanity_check(*d.lookup(key), stack(key))) {
                    return false;
                }
            }
            return true;
        } else if (type == "list") {
            for (auto &elt : *(val.to_list())) {
                if (!pickle_sanity_check(*elt, stack)) {
                    return false;
                }
            }
            return true;
        } else {
            xpub3_expr_t x;
            str repr;
            if (val.to_xdr(&x)) {
                auto o = xdr2json(x);
                if (o) {
                    repr = o->to_str();
                }
            }
            if (!repr) {
                repr = val.to_str();
            }

            ERROR << stack.to_str() << " cannot serialize type: "
                     << type << " value: " << repr << "\n";
            return false;
        }
    }

    // Dumps out a pub::expr into a value that can be read back in with lose of
    // type information.
    // (pub3::expr_t::to_string() looses some type information between
    // int/uint...)
    ptr<pub3::expr_t> pickle(const pub3::expr_t &v, const pickle_stack &stck) {
        xpub3_expr_t x;
        if (!pickle_sanity_check(v, stck)) {
            return nullptr;
        }
        if (!v.to_xdr(&x)) {
            ERROR << "Failed to convert to xdr.\n";
            return nullptr;
        }
        return xdr2json(x);
    }

    ptr<pub3::expr_t> pickle(ptr<const pub3::expr_t> e,
                             const pickle_stack &stck) {
        if (!e) {
            ERROR << "Null value.\n";
            return nullptr;
        }
        return pickle(*e, stck);
    }

    //--------------------------------------------------------------------------
    // ! Wrapper
    //--------------------------------------------------------------------------
    // Records a function's arguments and return value

    using pub3::str_opt_t;  // used in the PUB3_TO_STR_ARG macro
    class wrapped_fn_t :  public pub3::expr_t, public pub3::callable_t {
        ref<const pub3::expr_t> m_fn;
        ref<pub3::pub_tracer_t> m_tracer;
        str m_name;
        bool m_pure;

     protected:
        ptr<const pub3::callable_t> to_callable() const override {
            return mkref (this);
        }

     public:
        wrapped_fn_t(const str& name,
                     ptr<const pub3::expr_t> fn,
                     ref<pub3::pub_tracer_t> tracer,
                     bool pure):
            m_fn(fn), m_tracer(tracer), m_name(name), m_pure(pure) {}

        bool might_block() const final override {
            return m_fn->to_callable()->might_block();
        }

        str to_str(PUB3_TO_STR_ARG) const override final {
            return m_fn->to_str(sot);
        }

        bool to_xdr(xpub3_expr_t *x) const override final {
            return m_fn->to_xdr(x);
        }

        const char *get_obj_name() const override final {
            return m_fn->get_obj_name();
        }

        virtual ptr<const pub3::expr_t> eval_to_val(pub3::eval_t *e,
                                                    args_t args)
            const final override;
        virtual ptr<pub3::mref_t> eval_to_ref(pub3::eval_t *e, args_t args)
            const final override;
        virtual ptr<pub3::expr_t> eval_to_mval(pub3::eval_t *e, args_t args)
            const final override;
        virtual void pub_to_val(pub3::eval_t *p,
                                args_t args,
                                pub3::cxev_t,
                                CLOSURE) const final override;
        virtual void pub_to_ref(pub3::eval_t *p,
                                args_t a,
                                pub3::mrev_t ev,
                                CLOSURE) const final override;
        virtual void pub_to_mval(pub3::eval_t *p,
                                 args_t a,
                                 pub3::xev_t ev,
                                 CLOSURE) const final override;

        void record(const args_t &a, ptr<const pub3::expr_t> res,
                    pub3::eval_t *p) const;
    };

    //--------------------------------------------------------------------------

    ptr<const pub3::expr_t> to_expr(ptr<const pub3::expr_t> e) {
        return e;
    }

    ptr<const pub3::expr_t> to_expr(ptr<pub3::mref_t> e) {
        return e->get_value();
    }

    ptr<const pub3::expr_t> to_expr(ptr<pub3::expr_t> e) {
        return e;
    }

    //--------------------------------------------------------------------------

    void wrapped_fn_t::record(const args_t &a,
                              ptr<const pub3::expr_t> ret,
                              pub3::eval_t *p) const {
        if (m_tracer->failed) {
            return;
        }
        pickle_stack pstack(m_name);
        ptr<pub3::expr_dict_t> res = New refcounted<pub3::expr_dict_t>();
        ptr<pub3::expr_t> retval = pickle(ret, pstack("retval"));
        ptr<pub3::expr_list_t> args = New refcounted<pub3::expr_list_t>();
        if (!retval) {
            ERROR << "rec " << m_name << "() => failed to record\n";
            m_tracer->failed = true;
            return;
        }
        args->reserve(a->size());
        for (auto &e : *a) {
            ptr<pub3::expr_t> pe = pickle(*e, pstack("arg"));
            if (!pe) {
                ERROR << "rec " << m_name << "() => failed to record\n";
                m_tracer->failed = true;
                return;
            }
            args->push_back(pe);
        }
        res->insert("function", pub3::expr_str_t::alloc(m_name));
        res->insert("result", retval);
        res->insert("args", args);
        res->insert("location", pub3::locobj(*p));
        res->insert("pure", pub3::expr_bool_t::alloc(m_pure));

        m_tracer->trace->push_back(res);
    }

    ptr<const pub3::expr_t> wrapped_fn_t::eval_to_val(pub3::eval_t *e,
                                                      fn_args_t ai) const {
        const fn_args_t args = pub3::eval_args(e, ai);
        auto res = m_fn->to_callable()->eval_to_val(e, args);
        record(args, to_expr(res), e);
        return res;
    }

    ptr<pub3::mref_t> wrapped_fn_t::eval_to_ref(pub3::eval_t *e,
                                                fn_args_t ai) const {
        const fn_args_t args = pub3::eval_args(e, ai);
        auto res = m_fn->to_callable()->eval_to_ref(e, args);
        record(args, to_expr(res), e);
        return res;
    }

    ptr<pub3::expr_t> wrapped_fn_t::eval_to_mval(pub3::eval_t *e,
                                                 fn_args_t ai) const {
        const fn_args_t args = pub3::eval_args(e, ai);
        auto res = m_fn->to_callable()->eval_to_mval(e, args);
        record(args, to_expr(res), e);
        return res;
    }

    tamed
    void wrapped_fn_t::pub_to_val(pub3::eval_t *p,
                                  fn_args_t ai,
                                  pub3::cxev_t ev) const {
        tvars{
            ptr<const pub3::expr_t> res;
            ptr<pub3::expr_list_t> args;
        };
        args.alloc();
        twait { pub3::pub_args(p, ai, args, mkevent()); }
        twait { m_fn->to_callable()->pub_to_val(p, args, mkevent(res)); }
        record(args, to_expr(res), p);
        return ev->trigger(res);
    }

    tamed
    void wrapped_fn_t::pub_to_ref(pub3::eval_t *p,
                                  fn_args_t ai,
                                  pub3::mrev_t ev) const {
        tvars{
            ptr<pub3::mref_t> res;
            ptr<pub3::expr_list_t> args;
        };
        args.alloc();
        twait { pub3::pub_args(p, ai, args, mkevent()); }
        twait { m_fn->to_callable()->pub_to_ref(p, args, mkevent(res)); }
        record(args, to_expr(res), p);
        return ev->trigger(res);
    }

    tamed
    void wrapped_fn_t::pub_to_mval(pub3::eval_t *p,
                                   fn_args_t ai,
                                   pub3::xev_t ev)  const {
        tvars{
            ptr<pub3::expr_t> res;
            ptr<pub3::expr_list_t> args;
        };
        args.alloc();
        twait { pub3::pub_args(p, ai, args, mkevent()); }
        twait { m_fn->to_callable()->pub_to_mval(p, args, mkevent(res)); }
        record(args, to_expr(res), p);
        return ev->trigger(res);
    }

    //-------------------------------------------------------------------------
    // ! Environment wrapping...
    //-------------------------------------------------------------------------
    const char* NATIVE_FNNAME_PREAMBLE = "<compiled function> ";
    const char* RFN3 = "rfn3:";
    const char* NATIVE_FUNC_FILENAME = "/okcontent/lib/universal_functions.pub";

    // Extract the name of a native function
    str get_native_function_name(const pub3::expr_t &e) {
        str name = e.to_str();
        const size_t PLEN(strlen(NATIVE_FNNAME_PREAMBLE));
        if (!name
            || name.len() < PLEN
            || memcmp(name.cstr(), NATIVE_FNNAME_PREAMBLE, PLEN) != 0) {
            return str();
        }
        return (name.cstr() + PLEN);
    }

    ptr<pub3::expr_t> get_path(ptr<pub3::expr_t> e) { return e; }

    template<typename... Args>
    ptr<pub3::expr_t> get_path(ptr<pub3::expr_t> e,
                               const char *v,
                               Args... rest) {
        if (!e) {
            return nullptr;
        }
        auto d = e->to_dict();
        if (!d) {
            return nullptr;
        }
        return get_path(d->lookup(v), rest...);
    }

    // Extract the file a lambda was defined in...
    str get_lambda_source_file(const pub3::expr_t &e) {
        xpub3_expr_t x;
        if (!e.to_xdr(&x)) {
            return str();
        }
        ptr<pub3::expr_t> fname = get_path(xdr2json(x),
                                           "lambda",
                                           "filename",
                                           "val");
        if (!fname) {
            return str();
        }
        return fname->to_str();
    }

    struct wrap_res_t {
        ptr<pub3::expr_t> val;
        ptr<pub3::expr_dict_t> desc;
        void clear() {
            val  = nullptr;
            desc = nullptr;
        }
    };

    wrap_res_t wrap(ptr<pub3::expr_t> e,
                    ref<pub3::pub_tracer_t> tracer,
                    const bhash<str> &pure_funs) {
        wrap_res_t res;
        res.desc.alloc();
        ptr<const pub3::callable_t> c = e->to_callable();
        if (c) {
            const str name = get_native_function_name(*e);
            if (!name) {
                ERROR << "Found none wrappable value." << e->to_str()
                         << "\n";
                res.clear();
                return res;
            }
            if (name.len() > 5 && memcmp(RFN3, name.cstr(), 5) == 0) {
                str rfn3_name(name.cstr() + 5);
                // We need to trace non-purely functional stuff...
                if (rfn3_name == "rand"
                    || rfn3_name == "randsel"  // Note: randsel might take
                                               // non-serializable args in which
                                               // case will need to make a
                                               // custom wrapper.
                    || rfn3_name == "days_from_now"
                    || rfn3_name == "time_from_now"
                    || rfn3_name == "now"
                    || rfn3_name == "time_format"
                    || rfn3_name == "localtime"
                    || rfn3_name == "localtime_raw") {
                    // Go on to be wrapped normally.
                } else if (rfn3_name == "fork") {
                    res.val = New refcounted<fork_t>(tracer);
                    res.desc->insert("type", "replaced");
                    res.desc->insert("name", rfn3_name);
                    return res;
                } else if (rfn3_name == "shotgun") {
                    res.desc->insert("type", "replaced");
                    res.desc->insert("name", rfn3_name);
                    res.val = New refcounted<pub3::serial_shotgun_t>();
                    return res;
                } else {
                    // TODO: Optionally use a safe wrapper that checks that
                    // we get reproducible results during replay...
                    res.desc->insert("type", "native");
                    res.desc->insert("name", rfn3_name);
                    res.val = e;
                    return res;
                }
            }
            res.desc->insert("type", "wrapped");
            res.desc->insert("name", name);
            res.desc->insert
                ("might_block",
                 pub3::expr_bool_t::alloc(c->might_block()));
            const bool pure = pure_funs[name];
            res.desc->insert
                ("pure",
                 pub3::expr_bool_t::alloc(pure_funs[name]));
            res.val = New refcounted<wrapped_fn_t>(name, e, tracer, pure);
            return res;
        }
        ptr<pub3::expr_dict_t> d = e->to_dict();
        if (d) {
            auto fld_desc = pub3::expr_dict_t::alloc();
            auto res_val = pub3::expr_dict_t::alloc();
            for (str fld : *d) {
                wrap_res_t v = wrap(d->lookup(fld), tracer, pure_funs);
                if (!v.val || !v.desc) {
                    res.clear();
                    return res;
                }
                fld_desc->insert(fld, v.desc);
                res_val->insert(fld, v.val);
            }
            res.desc->insert("type", "dict");
            res.desc->insert("fields", fld_desc);
            res.val = res_val;
            return res;
        }
        ERROR << "Found a none wrappable value: " << e->to_str() << "\n";
        res.clear();
        return res;
    }

    wrap_res_t wrap_calls(const pub3::obj_dict_t &globals,
                          ptr<pub3::pub_tracer_t> tracer,
                          const bhash<str> &pure_funs) {
        wrap_res_t res;
        ptr<pub3::expr_dict_t> res_vals = globals.dict()->copy()->to_dict();
        auto res_descs = pub3::expr_dict_t::alloc();
        const auto *libs = pub3::singleton_t::get()->libraries();

        for (auto l : *libs) {
            for (auto fld : *l) {
                auto val (*((*l)[fld]));
                if (res_vals->lookup(fld)) {
                    ERROR << "Field "
                          << fld << " already exists in globals...\n";
                    res.clear();
                    return res;
                }
                auto wrapped = wrap(val, tracer, pure_funs);
                if (!wrapped.val || !wrapped.desc) {
                    res.clear();
                    return res;
                }
                res_vals->insert(fld, wrapped.val);
                res_descs->insert(fld, wrapped.desc);
            }
        }
        res.val = res_vals;
        res.desc = res_descs;
        return res;
    }

    ptr<pub3::expr_dict_t> pickle_globals(const pub3::obj_dict_t &globs) {
        auto retval = pub3::expr_dict_t::alloc();
        pickle_stack stack("globals");
        auto &globs_dict = *(globs.dict());
        for (str key : globs_dict) {
            auto val = globs_dict.lookup(key);
            auto pickled = pickle(val, stack(key));
            if (!pickled) {
                return nullptr;
            }
            retval->insert(key, pickled);
        }
        return retval;
    }

    ptr<pub3::expr_dict_t> pickle_universals(const pub3::expr_dict_t &univ) {
        auto retval = pub3::expr_dict_t::alloc();
        pickle_stack stack("universals");
        for (str key : univ) {
            auto val = univ.lookup(key);
            if (val->to_callable()
                && get_lambda_source_file(*val) == NATIVE_FUNC_FILENAME) {
                continue;
            } else {
                auto pickled = pickle(val, stack(key));
                if (!pickled) {
                    return nullptr;
                }
                retval->insert(key, pickled);
            }
        }
        return retval;
    }

}  // namespace

//------------------------------------------------------------------------------
// ! External interface
//------------------------------------------------------------------------------

ptr<pub3::pub_tracer_t> pub3::start_trace(pub3::obj_dict_t *m_obj, str file,
                                          const bhash<str> &pure_funs) {
    ptr<pub3::pub_tracer_t> tracer = new refcounted<pub3::pub_tracer_t>();
    wrap_res_t wrapped = wrap_calls(*m_obj, tracer, pure_funs);
    ptr<pub3::expr_t> globals = pickle_globals(*m_obj);
    ptr<pub3::expr_t> universals =
        pickle_universals(*pub3::singleton_t::get()->universals());
    if (wrapped.val && wrapped.desc && globals && universals) {
        tracer->universals = universals->to_dict();
        tracer->globals = globals->to_dict();
        tracer->file = file;
        tracer->libraries = wrapped.desc;
        *m_obj = wrapped.val->to_dict();
        return tracer;
    }
    return nullptr;
}

ptr<pub3::expr_t> pub3::end_trace(ptr<const pub3::pub_tracer_t> tracer,
                                  const str &out) {
    if (!tracer || tracer->failed) {
        return nullptr;
    }
    auto outval = pub3::expr_dict_t::alloc();
    auto univ_files = pub3::expr_list_t::alloc();
    univ_files->push_back(pub3::expr_str_t::alloc(NATIVE_FUNC_FILENAME));
    outval->insert("universal_files", univ_files);
    outval->insert("file", pub3::expr_str_t::alloc(tracer->file));
    outval->insert("globals", tracer->globals);
    outval->insert("universals", tracer->universals);
    outval->insert("trace", tracer->trace);
    outval->insert("libraries", tracer->libraries);
    outval->insert("output", out);
    return outval;
}

//------------------------------------------------------------------------------
