// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#ifndef _LIBPUB_PUB3PARSE_H_
#define _LIBPUB_PUB3PARSE_H_

#include "pub.h"
#include "parr.h"
#include "pub3expr.h"
#include "okformat.h"
#include "pub3expr.h"
#include "pub3obj.h"

namespace pub3 {

  class json_parser_t {
  public:
    static void set_output (ptr<expr_t> e);
    static ptr<expr_t> parse (const str &in);
  };

};


#endif /* _LIBPUB_PUB3OBJ_H_ */

