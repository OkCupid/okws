
#include "form.h"
#include "rxx.h"


static rxx email_rxx ("[^@]+@[a-zA-Z][a-zA-Z0-9._-]*\\.[a-zA-Z]{2,5}");
static rxx zip_rxx   ("[0-9]{5}(-[0-9]{4})?");

str
check_email (const str &in)
{
  if (email_rxx.match (in))
    return in;
  else
    return NULL;
}

str 
check_zipcode (const str &in)
{
  if (zip_rxx.match (in))
    return in;
  else
    return NULL;
}
