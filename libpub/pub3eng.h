// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub.h"
#include "pub3ast.h"
#include "pub3eval.h"

//=======================================================================
//
// pub3eng --- A runtime engine for pub command-line clients.
//
//=======================================================================

namespace pub3 {

  class eng_t {
  public:
    eng_t () : _opt (P_COPY_CONF), _ppt (NULL), _syntax_check (false) {}
    void init (int argc, char **argv, bool *gop, evi_t ev, CLOSURE);
    void run (evi_t ev, CLOSURE);
    void syntax_check (evi_t ev, CLOSURE);
    static void usage ();
    void main (int argc, char **argv, evi_t ev, CLOSURE);
  private:

    void run_pub (evi_t ev, CLOSURE);
    void check_files (evi_t ev, CLOSURE);
    void run_file (str s, evi_t ev, CLOSURE);
    void check_file (str s, evi_t ev, CLOSURE);

    int _opt;
    str _jaildir;
    str _config;
    ptr<pub3::pub_parser_t> _ppt;
    ptr<pub3::jailer_t> _jailer;
    ptr<pub3::local_publisher_t> _pub;
    vec<str> _files;
    str _argfile;
    pub3::obj_dict_t _dict;
    vec<str> _print_vars;
    bool _syntax_check;
  };

};

//=======================================================================
