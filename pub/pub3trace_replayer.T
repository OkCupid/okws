// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#include "amisc.h"
#include "pub3.h"
#include "pub3parse.h"
#include "pub3ast.h"
#include "pub3tracer.h"
#include "json_rpc.h"
#include "okrfn.h"
#include <iostream>
#include <fstream>

extern "C" {
#include <argp.h>
#include <linux/limits.h>
}

#define FATAL(e)                                                        \
    do {                                                                \
        warn << "ERROR: at line " << __LINE__ <<  " in " << __func__    \
             << ": " << e << "\n";                                      \
        exit(1);                                                        \
    } while (false);


namespace {

    struct cli_arguments_t {
        str root;
        str file;
        bool sanity_check = true;
        bool dump_file_list = false;
        unsigned int num_runs = 50;
    };

    //--------------------------------------------------------------------------

    str loc_to_str(ptr<const expr_t> loc) {
        auto ll = loc->to_list();
        if (!ll) {
            return loc->to_str();
        }
        strbuf res("[");
        bool first = true;
        for (auto v : *ll) {
            if (first) {
                res << "\n  ";
                first = false;
            } else {
                res << ",\n  ";
            }
            res << v->to_str();
        }
        res << "\n]";
        return str(res);
    }

    str args_to_str(const pub3::callable_t::args_t &args, bool async = false) {
        strbuf res((async)? "(*" : "(");
        bool first = true;
        for (auto &a : *args) {
            if (first) {
                res << ", ";
                first = false;
            }
            res << a->to_str();
        }
        res << ((async)? "*)" : ")");
        return str(res);
    }

    //--------------------------------------------------------------------------

    struct call_rec {
        ptr<const pub3::expr_list_t> args;
        ptr<const pub3::expr_t> result;
    };

    struct traced_call : public call_rec {
        str function;
        ptr<const pub3::expr_t> location;
    };

    class pure_calls {
        qhash<str, ref<vec<call_rec>>> m_calls;
     public:
        void add(str, const call_rec &call);
        ptr<const pub3::expr_t> find(str fun,
                                     ptr<const pub3::expr_list_t> args) const;
    };

    void pure_calls::add(str fun, const call_rec &call) {
        ptr<vec<call_rec>> vals = m_calls[fun];
        if (!vals) {
            vals.alloc();
            m_calls.insert(fun, vals);
        } else {
            for (auto val : *vals) {
                if (pub3::expr_EQ_t::eval_static(val.args, call.args)) {
                    if (pub3::expr_EQ_t::eval_static(val.result, call.result)) {
                        // We already have him in there...
                        return;
                    } else {
                        FATAL("mismatched results for " << fun
                              << args_to_str(call.args)
                              << "got:\n"
                              << call.result->to_str()
                              << "\nbut already had recorded:\n"
                              << val.result->to_str());
                    }
                }
            }
            vals->push_back(call);
        }
    }

    ptr<const pub3::expr_t>
    pure_calls::find(str fun, ptr<const pub3::expr_list_t> args) const {
        const auto vals = m_calls[fun];
        if (vals) {
            for (auto val : *vals) {
                if (pub3::expr_EQ_t::eval_static(val.args, args)) {
                    return val.result;
                }
            }
        }
        FATAL("Could not find result for " << fun << args_to_str(args));
    }

    //--------------------------------------------------------------------------

    struct replay_info {
        size_t pos = 0;
        bool sanity_check = true;
        vec<traced_call> calls;
        pure_calls pure;
        void reset() { pos = 0; }
    };

    //  * Normal calls:
    // When we replay a file we expect all the native functions to be called
    // in the same order they were called in on the server that recorded the
    // trace. [calls] is used to emulate those function calls. We use the
    // function name, the args and the locations as sanity checks.
    //  * Pure calls:
    // Pure calls is a hack where we mark some native functions as purely
    // functional. Every time we call one of those functions we will just
    // search for an instance where it was called with the same arguments in the
    // pure_calls qhash.
    // Ideally when replaying a file we should get all the recorded calls in the
    // same order. Sadly the order the values in global dictionaries are
    // initialized is highly unstable. This is fine as long as they use only
    // `pure` functions

    typedef pub3::callable_t::args_t fn_args_t;

    //--------------------------------------------------------------------------
    // ! Replacement pub functions
    //--------------------------------------------------------------------------
    using pub3::str_opt_t;  // used in the PUB3_TO_STR_ARG macro

    //--------------------------------------------------------------------------

    class base_replay_fn_t : public pub3::expr_t, public pub3::callable_t {
     protected:
        str m_name;
        ref<replay_info> m_ri;
        virtual ptr<const pub3::expr_t> run(pub3::eval_t *e, args_t args) const
        = 0;
        bool m_might_block;

        base_replay_fn_t(str name, ref<replay_info> ri, bool might_block) :
            m_name(name), m_ri(ri), m_might_block(might_block) {}

        ptr<const pub3::callable_t> to_callable() const final override {
            return mkref (this);
        }

     public:
        bool might_block() const final override {
            return m_might_block;
        }

        str to_str(str_opt_t o) const override final {
            strbuf b("<compiled function> %s", m_name.cstr());
            return pub3::json::quote (b, o);
        }

        bool to_xdr(xpub3_expr_t *x) const override final {
            FATAL("Not implemented");
            // return false;
        }

        const char *get_obj_name() const override final {
            FATAL("Not implemented");
            // return "replay_fn";
        }

        virtual ptr<const pub3::expr_t> eval_to_val(pub3::eval_t *e,
                                                    args_t args)
            const final override;

        virtual void pub_to_val(pub3::eval_t *p, args_t args, pub3::cxev_t,
                                CLOSURE)
            const final override;
    };

    ptr<const pub3::expr_t> base_replay_fn_t::eval_to_val(pub3::eval_t *e,
                                                     args_t ai) const {
        const args_t args = pub3::eval_args(e, ai);
        return run(e, args);
    }

    tamed
    void base_replay_fn_t::pub_to_val(pub3::eval_t *p,
                                 fn_args_t ai,
                                 pub3::cxev_t ev) const {
        tvars{
            ptr<const pub3::expr_t> res;
            ptr<pub3::expr_list_t> args;
        };
        args.alloc();
        twait { pub3::pub_args(p, ai, args, mkevent()); }
        res = run(p, args);
        return ev->trigger(res);
    }

    //--------------------------------------------------------------------------
    // !! Normal replay function
    //--------------------------------------------------------------------------

    class replay_fn_t : public base_replay_fn_t {
        virtual ptr<const pub3::expr_t> run(pub3::eval_t *e, args_t args)
            const override final;

     public:
        replay_fn_t(str name, ref<replay_info> ri, bool might_block) :
            base_replay_fn_t(name, ri, might_block){}
    };

    ptr<const pub3::expr_t> replay_fn_t::run(pub3::eval_t *e,
                                             args_t args) const {
        if (m_ri->pos >= m_ri->calls.size()) {
            FATAL("Replay out of bounds");
        }
        const traced_call &call = m_ri->calls[m_ri->pos];

        if (m_ri->sanity_check) {

            auto loc = pub3::locobj(*e);

            if (m_name != call.function ||
                !pub3::expr_EQ_t::eval_static(call.args, args)) {
                e->output_err_stacktrace("stack", pub3::P_ERR_EVAL);
                FATAL("mismatched call [" << m_ri->pos <<"] got: " << m_name
                      << args_to_str(args)
                      << " expected: "
                      << call.function
                      << args_to_str(call.args)
                      << "\nLocation:\n"
                      << loc_to_str(loc)
                      << "\nrecorded:\n"
                      << loc_to_str(call.location));
            }

            if (!pub3::expr_EQ_t::eval_static(call.location, loc)) {
                FATAL("mismatched locations for " << call.function
                      << " got:\n"
                      << loc_to_str(loc)
                      << "\nexpected:\n"
                      << loc_to_str(call.location));
            }
        }

        m_ri->pos++;
        return call.result;
    }

    //--------------------------------------------------------------------------
    // !! Pure replay function
    //--------------------------------------------------------------------------

    class pure_replay_fn_t : public base_replay_fn_t {
        virtual ptr<const pub3::expr_t> run(pub3::eval_t *e, args_t args)
            const override final;
     public:
        pure_replay_fn_t(str name, ref<replay_info> ri, bool might_block) :
            base_replay_fn_t(name, ri, might_block){}
    };

    ptr<const pub3::expr_t> pure_replay_fn_t::run(pub3::eval_t *e,
                                                  args_t args) const {
        return m_ri->pure.find(m_name, args);
    }

    //--------------------------------------------------------------------------
    // !! Other
    //--------------------------------------------------------------------------

    class fork_t :  public pub3::patterned_fn_t {
     public:
        fork_t() : patterned_fn_t("rfn3", "fork", "F") {}

        ptr<const expr_t>
        v_eval_2(pub3::eval_t *, const vec<arg_t> &) const {
            FATAL("Someone called fork...");
        }
    };

    //--------------------------------------------------------------------------
    // ! Trace file reading
    //--------------------------------------------------------------------------
    str read_file(const str& fname) {
        std::ifstream file(fname.cstr(), std::ios::in|std::ios::ate);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            mstr buf(size);
            file.seekg(0, std::ios::beg);
            file.read(buf.cstr(), size);
            file.close();
            return str(buf);
        }
        return str();
    }

    ptr<pub3::expr_t> parse_json_file(const str &fname) {
        str content = read_file(fname);
        if (!content) {
            return nullptr;
        }
        return pub3::json_parser_t::parse(content);
    }

    struct recorded_trace {
        vec<str> univ_files;
        str file;
        ptr<pub3::expr_dict_t> globals;
        ptr<pub3::expr_dict_t> universals;
        str expected_output;
        ptr<replay_info> trace;
    };

#define NOT_NULL(_expr) ({                                              \
            auto v = _expr;                                             \
            if (v == nullptr) {                                         \
                FATAL(#_expr << " was null.");                          \
            }                                                           \
            v;                                                          \
        })


    ptr<pub3::expr_t> find_native_func(const str &name) {
        static auto rfn3lib = rfn3::lib_t::alloc();
        static auto rfn3 = rfn3lib->bind();
        auto res = (*rfn3)[name];
        if (res) {
            return *res;
        }
        FATAL("Could not find native function: " << name);
    }

    ptr<pub3::expr_t> read_lib(ptr<pub3::expr_t> arg, ptr<replay_info> ri) {
        ptr<pub3::expr_dict_t> dict = NOT_NULL(NOT_NULL(arg)->to_dict());
        const str type = NOT_NULL(dict->lookup("type"))->to_str();
        if (type == "wrapped") {
            str name = NOT_NULL(dict->lookup("name"))->to_str();
            bool might_block = NOT_NULL(dict->lookup("might_block"))->to_bool();
            bool pure = NOT_NULL(dict->lookup("pure"))->to_bool();
            if (pure) {
                return New refcounted<pure_replay_fn_t>(name, ri, might_block);
            } else {
                return New refcounted<replay_fn_t>(name, ri, might_block);
            }
        } else if (type == "dict") {
            ptr<pub3::expr_dict_t> retval;
            retval.alloc();
            ptr<pub3::expr_dict_t> flds =
                NOT_NULL(NOT_NULL(dict->lookup("fields"))->to_dict());
            for (str fname : *flds) {
                retval->insert(fname, read_lib(flds->lookup(fname), ri));
            }
            return retval;
        } else if (type == "replaced") {
            str name = NOT_NULL(dict->lookup("name"))->to_str();
            if (name == "fork") {
                return New refcounted<fork_t>();
            } else if (name == "shotgun") {
                return New refcounted<pub3::serial_shotgun_t>();
            }
            FATAL("Unknown repaced function" << name);
        } else if (type == "native") {
            str name = NOT_NULL(dict->lookup("name"))->to_str();
            return find_native_func(name);
        }
        FATAL("Unknown type: " <<  type);
    }

    // gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
    // Using a statement expr for fun and profit
    // This macro will print an error and exit the function if its argument
    // is null; otherwise it'll just return its argument
    void parse_v(ptr<pub3::expr_t> e, recorded_trace *out) {
        ptr<pub3::expr_dict_t> dict = NOT_NULL(NOT_NULL(e)->to_dict());

        ptr<pub3::expr_t> univ_files =
            NOT_NULL(dict->lookup("universal_files"));
        for (auto &file : *(NOT_NULL(univ_files->to_list()))) {
            out->univ_files.push_back(file->to_str());
        }

        out->file = NOT_NULL(dict->lookup("file"))->to_str();
        out->expected_output = NOT_NULL(dict->lookup("output"))->to_str();

        out->globals.alloc();
        ptr<pub3::expr_dict_t> globals =
            NOT_NULL(NOT_NULL(dict->lookup("globals"))->to_dict());
        for (const str& key : *globals) {
            ptr<expr_t> val = globals->lookup(key);
            xpub3_expr_t x;
            if (!json2xdr(x, val)) {
                FATAL("Json2xdr failed on: globals::"<< key);
            }
            out->globals->insert(key, NOT_NULL(pub3::expr_t::alloc(x)));
        }

        out->universals.alloc();
        ptr<pub3::expr_dict_t> universals =
            NOT_NULL(NOT_NULL(dict->lookup("universals"))->to_dict());
        for (const str& key : *universals) {
            ptr<expr_t> val = universals->lookup(key);
            xpub3_expr_t x;
            if (!json2xdr(x, val)) {
                FATAL("Json2xdr failed on universals::"<< key);
            }
            out->universals->insert(key, NOT_NULL(pub3::expr_t::alloc(x)));
        }

        ptr<pub3::expr_t> trace = NOT_NULL(dict->lookup("trace"));

        out->trace.alloc();
        for (auto &e : *(NOT_NULL(trace->to_list()))) {
            traced_call call;
            ptr<pub3::expr_dict_t> d = NOT_NULL(e->to_dict());
            call.function = NOT_NULL(d->lookup("function"))->to_str();
            call.location =  NOT_NULL(d->lookup("location"));
            {
                xpub3_expr_t x;
                if (!json2xdr(x, NOT_NULL(d->lookup("result")))) {
                    FATAL("json2xdr failed on:" << call.function);
                }
                call.result = NOT_NULL(pub3::expr_t::alloc(x));
            }
            ptr<pub3::expr_t> args = NOT_NULL(d->lookup("args"));
            ptr<pub3::expr_list_t> res_args;
            res_args.alloc();
            call.args.alloc();
            for (auto &arg : *(NOT_NULL(args->to_list()))) {
                xpub3_expr_t x;
                if (!json2xdr(x, arg)) {
                    FATAL("json2xdr failed on:" << call.function);
                }
                res_args->push_back(NOT_NULL(pub3::expr_t::alloc(x)));
            }
            call.args = res_args;
            bool pure = NOT_NULL(d->lookup("pure"))->to_bool();
            if (pure) {
                out->trace->pure.add(call.function, call);
            } else {
                out->trace->calls.push_back(call);
            }
        }

        ptr<pub3::expr_dict_t> libraries = NOT_NULL
            (NOT_NULL(dict->lookup("libraries"))->to_dict());

        for (auto &key : *libraries) {
            if (out->globals->lookup(key)) {
                FATAL(key << " already exists in globals");
            }
            ptr<pub3::expr_t> new_val = read_lib(*(*libraries)[key],
                                                 out->trace);
            out->globals->insert(key, new_val);
        }
    }

#undef NOT_NULL

    //--------------------------------------------------------------------------
    // ! Custom publisher
    //--------------------------------------------------------------------------

    class replay_publisher_t : public pub3::abstract_publisher_t {
     public:
        replay_publisher_t(str root);
        virtual ~replay_publisher_t() {}
        virtual void getfile(str fn, pub3::getfile_ev_t ev, pub3::opts_t o = 0)
            override final;
        vec<str> list_parsed_files() const;
     protected:

        virtual bool is_remote () const override final {
            return false;
        }

        ptr<pub3::pub_parser_t> _parser;
        qhash<str, ref<pub3::file_t>> _cache;
        ptr<pub3::jailer_t> _jailer;
    };

    replay_publisher_t::replay_publisher_t(str root) :
        _parser (pub3::pub_parser_t::alloc()),
        _jailer (pub3::jailer_t::alloc()) {
        _jailer->setjail(pub3::JAIL_NONE, root);
    }

    void
    replay_publisher_t::getfile(str jfn, pub3::getfile_ev_t ev,
                                pub3::opts_t o) {
        xpub_status_t res (XPUB_STATUS_OK);
        ptr<pub3::file_t> f = _cache[jfn];

        if (f) {
            return ev->trigger (res, f);
        } else {
            str rfn = _jailer->jail2real (jfn);
            str em;
            pub3::parse_ret_t pr;

            struct ::stat sb;
            ptr<pub3::fhash_t> hsh = pub3::file2hash(rfn, &sb);

            ptr<pub3::metadata_t> meta =
                New refcounted<pub3::metadata_t>(jfn, rfn, hsh);
            if (_parser->parse(meta, &pr, o)) {
                f = pr.file();
                assert(f);
                _cache.insert(jfn, f);;
            }
            if (!pr.ok()) {
                assert(!f);
                pr.to_xdr (&res);
                warn << jfn << ": Parse failed!\n";
            }
        }
        return ev->trigger(res, f);
    }

    vec<str> replay_publisher_t::list_parsed_files() const {
        vec<str> res;
        for (str s : _cache) {
            res.push_back(s);
        }
        return res;
    }

    //--------------------------------------------------------------------------
    // ! Eval
    //--------------------------------------------------------------------------

    // void pre_eval(const recorded_trace &tr, evv_t, CLOSURE);
    // tamed void pre_eval(recorded_trace &tr, evv_t ev) {
    // }
    typedef event<ptr<replay_publisher_t>>::ptr evpub_t;

    void eval(ptr<recorded_trace> tr,
              ptr<replay_publisher_t> pub,
              evv_t,
              CLOSURE);

    void main2(cli_arguments_t, CLOSURE);

    void get_publisher(const recorded_trace &tr, str root,
                       evpub_t pub, CLOSURE);

    tamed void main2(cli_arguments_t cli) {
       tvars {
           ptr<pub3::expr_t> file;
           size_t i;
           ptr<recorded_trace> tr;
           ptr<replay_publisher_t> pub;
       }
       zinit(false);
       tr.alloc();
       file = parse_json_file(cli.file);
       parse_v(file, tr);
       twait { get_publisher(*tr, cli.root, mkevent(pub)); }

       tr->trace->sanity_check = cli.sanity_check;

       for (i = 0; i < cli.num_runs; ++i) {
           twait { eval(tr, pub, mkevent()); }
       }

       if (cli.dump_file_list) {
           warn << "Parsed files:\n";
           for (auto s : pub->list_parsed_files()) {
               warn << s << "\n";
           }
       }
       exit(0);
    }

    //--------------------------------------------------------------------------

    tamed void get_publisher(const recorded_trace &tr, str root, evpub_t ev) {
        tvars {
            size_t i;
            ptr<replay_publisher_t> pub;
            bool ok;
        }
        pub.alloc(root);

        for (i = 0; i < tr.univ_files.size(); ++i) {
            twait { pub->run_cfg(tr.univ_files[i],
                                 mkevent(ok),
                                 tr.universals,
                                 pub3::P_COPY_CONF | pub3::P_EXIT_ON_ERROR); }
            if (!ok) {
                FATAL("Failed to load univ file: " << tr.univ_files[i]);
            }
            if (tr.trace->pos != 0) {
                FATAL("Some of the univ calls were mocked. "
                      "We are doomed.")
            }
        }

        ev->trigger(pub);
    }

    //--------------------------------------------------------------------------

    tamed void eval(ptr<recorded_trace> tr,
                    ptr<replay_publisher_t> pub,
                    evv_t ev) {
        tvars {
            zbuf out;
            bool ok;
        }

        twait {
            pub->run(&out,
                     tr->file,
                     mkevent(ok),
                     tr->globals->copy()->to_dict(),
                     pub3::P_EXIT_ON_ERROR);
        }

        if (tr->trace->sanity_check) {
            strbuf b;
            out.output(&b);
            str res(b);

            if (res != tr->expected_output) {
                FATAL("Output wasn't the expected value.");
            }
        }

        tr->trace->reset();

        ev->trigger();
    }
}  // namespace

//------------------------------------------------------------------------------
// ! CLI
//------------------------------------------------------------------------------

extern "C" {
  const char *argp_program_version =
    "pub_trace_replayer";

  const char *argp_program_bug_address =
    "<till@okcupid.com>";
}

namespace {
    const char doc[] =
        "Replay a recorded pub trace.";

    const char arg_doc[] =
        "FILE [PUB_ROOT]";

    /* The options we understand. */
    const struct argp_option options[] = {
        {"n-runs", 'n', "INT", 0, "The number of iterations to run the trace "
         "through (default 50).", 0 },

        {"no-sanity-check", 'S',  nullptr, 0,
         "Do not run any sanity checks before/after the runs.", 0 },

        {"dump-file-list", 'l',  nullptr, 0,
         "After running list all the files that were parsed.", 0 },

        { }
    };


    error_t
    parse_opt (int key, char *arg, struct argp_state *state)
    {
        /* Get the input argument from argp_parse, which we
           know is a pointer to our arguments structure. */
        auto *cli = reinterpret_cast<cli_arguments_t*>(state->input);

        switch (key) {
        case 'S':
            cli->sanity_check = false;
            break;
        case 'l':
            cli->dump_file_list = true;
            break;
        case 'n':
            {
                char *endptr = nullptr;
                auto v = strtoul(arg, &endptr, 10);
                if (*endptr != '\000')
                    argp_usage(state);
                cli->num_runs = static_cast<unsigned int>(v);
                break;
            }
        case ARGP_KEY_ARG:
            if (cli->file) {
                if (cli->root) {
                    argp_usage(state);
                } else {
                    cli->root = arg;
                }
            }else {
                cli->file = arg;
            }
            break;
        case ARGP_KEY_END:
            if (!cli->file) {
                /* Not enough arguments. */
                argp_usage (state);
            }
            if (!cli->root) {
                char current_path[PATH_MAX];
                char *path;
                do {
                    path = getcwd(current_path, PATH_MAX);
                } while(path == nullptr && errno == EINTR);
                if (path==nullptr) {
                    FATAL("While trying to getcwd():" << strerror(errno));
                }
                cli->root=path;
            }
        default:
            return ARGP_ERR_UNKNOWN;
        }
        return 0;
    }

    const struct argp argspecs = { options, parse_opt, arg_doc, doc, 0, 0, 0 };

}  // namespace

int main(int argc, char** argv)  {
    cli_arguments_t cli;
    argp_parse (&argspecs, argc, argv, 0, 0, &cli);
    main2(cli);
    amain();
    return 0;
}
