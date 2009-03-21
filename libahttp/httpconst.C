
#include "httpconst.h"
#include "async.h"

const char *rfc_date_fmt (rfc_number_t rfc)
{
  const char *out = NULL;
  switch (rfc) {
  case RFC_1036:
    out = "%A, %d-%b-%Y %T GMT";
    break;
  case RFC_1123:
    out = "%a, %d %b %Y %H:%M:%S GMT";
    break;
  default:
    break;
  }
  return out;
}

//-----------------------------------------------------------------------

namespace httpconst {

  bool is_redirect (int status) { return (status >= 300 && status < 400); }
  bool is_error (int status) { return (status >= 400); }

};

//-----------------------------------------------------------------------
