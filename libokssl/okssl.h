
// -*-c++-*-

#pragma once
#include <openssl/opensslv.h>
#include "oksslproxy.h"

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
# define OPENSSL_CONST const
#else
# define OPENSSL_CONST 
#endif

