
#include "pub.h"
#include "gmtoff.h"

//=======================================================================

bool
global_gmt_offset_t::get (long *val, time_t freshness)
{
  bool rc = false;
  if (_when_updated && _when_updated >= freshness) {
    *val = _val;
    rc = true;
  }
  return rc;
}

//-----------------------------------------------------------------------

void
global_gmt_offset_t::set (long v)
{
  _val = v;
  _when_updated = okwstime ();
}

//-----------------------------------------------------------------------

global_gmt_offset_t global_gmt_offset;

//-----------------------------------------------------------------------

