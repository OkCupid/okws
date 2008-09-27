
// -*-c++-*-

#ifndef __SSL_OKSSLUTIL_H__
#define __SSL_OKSSLUTIL_H__

#include "async.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace okssl {

  bool ssl_ok (int rc);
  bool init_ssl_internals ();
  void ssl_complain (const str &s);

};

#endif /* __SSL_OKSSLUTIL_H_ */
