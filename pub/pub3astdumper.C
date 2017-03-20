// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#include "pub3.h"
#include "pub3parse.h"
#include "pub3ast.h"
#include "pub3file.h"
#include "pub_parse.h"
#include "json_rpc.h"
#include <iostream>

extern "C" {
#include <stdio.h>
#include <argp.h>
}

extern void flex_cleanup();

namespace {

//------------------------------------------------------------------------------
//! Parsing
//------------------------------------------------------------------------------


    class parser_t : public pub3::pub_parser_t {
    public:
        // parse stdin
        bool parse(pub3::parse_ret_t *r, str jfn = nullptr);

        // parse a file
        bool parse(const str filename, pub3::parse_ret_t *r,
                   str jfn = nullptr);
    };

    bool parser_t::parse(pub3::parse_ret_t *r, str jfn) {
        if (!jfn) {
            jfn = "stdin";
        }
        ptr<pub3::fhash_t> hash = New refcounted<pub3::fhash_t>();
        ptr<pub3::metadata_t> meta =
            New refcounted<pub3::metadata_t>("stdin", jfn, hash);
        ptr<pub3::parser_t> old = current ();
        set_current (mkref (this));

        _location.set_filename (jfn);
        _ret = r;

        yy_buffer_state *yb = yy_create_buffer (stdin, ok_pub3_yy_buffer_size);
        yy_push_html_state ();
        yy_switch_to_buffer (yb);
        yyparse ();
        flex_cleanup ();
        r->set_file (pub3::file_t::alloc (meta, _out, 0));
        _out = NULL;

        set_current (old);
        return r->ok();
    }

    bool parser_t::parse(const str rfn,
                         pub3::parse_ret_t *r,
                         str jfn) {
        if (!jfn) {
            jfn = rfn;
        }
        struct ::stat sb;
        ptr<pub3::fhash_t> hsh = pub3::file2hash(rfn, &sb);

        ptr<pub3::metadata_t> meta =
            New refcounted<pub3::metadata_t>(jfn, rfn, hsh);

        return pub3::pub_parser_t::parse(meta, r, 0);
    }

//------------------------------------------------------------------------------

    bool run(const char *rfn, const char *jfn) {
        zinit(false, 0);
        pub3::parse_ret_t pr;
        ptr<parser_t> parser = New refcounted<parser_t>();
        if (rfn) {
            std::cout << "here!" << std::endl;
            parser->parse(rfn, &pr, jfn);
        } else {
            parser->parse(&pr, jfn);
        }
        if (!pr.ok()) {
            return false;
        }

        xpub3_file_t x;
        pr.file()->to_xdr(&x);
        std::cout << xdr2json(x)->to_str().cstr() << std::endl;
        return true;
    }

}  // namespace

//------------------------------------------------------------------------------
//! CLI
//------------------------------------------------------------------------------

extern "C" {
  const char *argp_program_version =
    "pub3astdumper";

  const char *argp_program_bug_address =
    "<till@okcupid.com>";
}

namespace {
    const char doc[] =
        "Dump out a pub3 AST as json.\vIf no file is provided will parse from "
        "stdin.";

    const char arg_doc[] =
        "[FILE]";

    /* Used by main to communicate with parse_opt. */
    struct arguments {
        const char *jfn = nullptr;
        const char *rfn = nullptr;
    };

    const struct argp_option options[] = {
        {"name", 'n', "STRING", 0, "The name of the file used for error "
         "reporting and position information", 0 },

        { }
    };


    error_t
    parse_opt (int key, char *arg, struct argp_state *state)
    {
        /* Get the input argument from argp_parse, which we
           know is a pointer to our arguments structure. */
        auto *cli = reinterpret_cast<arguments*>(state->input);

        switch (key) {
        case 'n':
            cli->jfn = arg;
            break;
        case ARGP_KEY_ARG:
            if (cli->rfn) {
                // Too many arguments
                argp_usage(state);
                exit(1);
            }
            cli->rfn = arg;
            break;
        case ARGP_KEY_END:
            break;
        default:
            return ARGP_ERR_UNKNOWN;
        }
        return 0;
    }

    const struct argp argspecs = { options, parse_opt, arg_doc, doc, 0, 0, 0 };

}  // namespace

int main(int argc, char** argv)  {
    make_sync(0);
    make_sync(1);
    make_sync(2);
    arguments cli;
    argp_parse (&argspecs, argc, argv, 0, 0, &cli);
    if (!run(cli.rfn, cli.jfn)) {
        // TODO: error message
        std::cerr << "Parse failed!" << std::endl;
        exit(1);
    }
    return 0;
}
