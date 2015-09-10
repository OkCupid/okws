// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#include "pub3.h"
#include "pub3hilev.h"
#include "pub3lib.h"
#include "pub3env.h"
#include "okrfn.h"
#include <iostream>
#include <fstream>

extern "C" {
#include <argp.h>
#include <libgen.h>
}

namespace dbg {

    using pub3::eval_t;

    namespace {
        // Referenced in the macros PUB3_COMPILED_FN
        const char *libname = "dbglib";
    }

    PUB3_COMPILED_FN_DOC(dump_env_stack, "");

    ptr<const pub3::expr_t>
    dump_env_stack_t::v_eval_2 (pub3::eval_t *p, const vec<arg_t> &args) const {
        const pub3::env_t::stack_t &stack = p->env()->stack();
        for (const pub3::env_t::stack_layer_t &layer : stack) {
            warn << pub3::env_t::layer_type_to_str(layer._typ);
            if (layer._typ !=  pub3::env_t::LAYER_LIBRARY
                && layer._typ != pub3::env_t::LAYER_UNIVERSALS
                && layer._typ != pub3::env_t::LAYER_GLOBALS) {
                if (!layer._bindings) {
                    warn << ": null";
                } else {
                    warn << ": {";
                    auto v = *layer._bindings->iter();
                    const str *k;
                    do {
                        k = v.next();
                        if (k) {
                            warn << " " << *k;
                        }
                    } while (k);
                    warn << " }";
                }
            } else {
                warn << ": { .. }";
            }
            warn << "\n";
        }
        return pub3::expr_null_t::alloc();
    }


    const str dump_env_stack_t::DOCUMENTATION =
        "Prints out the local environment stack. Useful for internal debuging "
        "purposes";

    class lib_t : public pub3::library_t {
    public:
        lib_t();
        static ptr<lib_t> alloc() { return New refcounted<lib_t> (); }
    };

    lib_t::lib_t () : library_t ()
    {

#define F(f)                                            \
        _functions.push_back (New refcounted<f##_t> ())
        F(dump_env_stack);
#undef F
    }

  //-----------------------------------------------------------------------
}

tamed static void main2(str fname, bool jailed) {
  tvars {
    zbuf out;
    bool ok;
    str root;
    ptr<pub3::local_publisher_t> pub;
    char *buf;
  }
  zinit(false);

  pub = New refcounted<pub3::local_publisher_t>();

  pub3::library_t::import(rfn3::lib_t::alloc());
  pub3::library_t::import(dbg::lib_t::alloc());

  if (jailed) {
    buf = strndup(fname.cstr(), fname.len());
    root = dirname(buf);
    free(buf);
    if (root != ".") {
      buf = strndup(fname.cstr(), fname.len());
      fname = basename(buf);
      free(buf);
      auto jailer = pub3::jailer_t::alloc();
      jailer->setjail(pub3::JAIL_NONE, root);
      pub->set_jailer(jailer);
    }
  }

  twait { pub->run(&out, fname, mkevent(ok)); }

  strbuf b;
  out.output(&b);
  make_sync(0);
  make_sync(1);
  if (b.len() == 0) {
    b << "\n";
  }
  b.tosuio ()->output (1);
  exit(ok ? 0 : 1);
}

//------------------------------------------------------------------------------
// ! CLI
//------------------------------------------------------------------------------

extern "C" {
  const char *argp_program_version =  "pub3";

  const char *argp_program_bug_address = "<till@okcupid.com>";
}

namespace {
    struct cli_arguments_t {
        str file;
        bool jailed = false;
    };

    const char doc[] =
        "Naive pub interpreter.";

    const char arg_doc[] =
        "FILE";

    const struct argp_option options[] = {
        { "jailed", 'j', nullptr, 0,
          "evaluate the file with a jailer rooted at the file's dirname", 0 },
        {}
    };


    error_t
    parse_opt (int key, char *arg, struct argp_state *state)
    {
        /* Get the input argument from argp_parse, which we
           know is a pointer to our arguments structure. */
        cli_arguments_t *cli = reinterpret_cast<cli_arguments_t*>(state->input);

        switch (key) {
        case 'j':
            cli->jailed = true;
            break;
        case ARGP_KEY_ARG:
            if (cli->file) {
                argp_usage(state);
            }
            cli->file = arg;
            break;
        case ARGP_KEY_END:
            if (!cli->file) {
                /* Not enough arguments. */
                argp_usage (state);
            }
            break;
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
    main2(cli.file, cli.jailed);
    amain();
    return 0;
}
