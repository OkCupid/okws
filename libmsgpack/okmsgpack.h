// -*-c++-*-

#pragma once

#include "pub3.h"

namespace msgpack {
  ptr<pub3::expr_t> decode (str m);
  str encode (ptr<const pub3::expr_t> x);
};
