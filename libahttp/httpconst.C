
#include "httpconst.h"
#include "async.h"

const char *rfc_date_fmt (rfc_number_t rfc)
{
  const char *out = NULL;
  switch (rfc) {
  case RFC_1036:
    out = "%A, %d-%b-%Y %T %Z";
    break;
  case RFC_1123:
    out = "%a, %d %b %Y %H:%M:%S %Z";
    break;
  default:
    break;
  }
  return out;
}

//-----------------------------------------------------------------------

bool 
http_is_redirect (int status)
{
  return (status >= 300 && status < 400);
}
