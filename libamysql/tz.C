
#include "amysql.h"
#include "mystmt.h"

#define TZWARN(x) TWARN("timezone: " << x)

bool
tz_corrector_t::prepare ()
{
  const char *qry = 
    "SELECT UNIX_TIMESTAMP(NOW()) - "
    "UNIX_TIMESTAMP(UTC_TIMESTAMP())";
  
  bool rc = true;
  if (!(_sth = _thr->prepare (qry))) {
    rc = false;
    TZWARN ("failed to prepare query: " << qry);
  }
  return rc;
}

bool
tz_corrector_t::run ()
{
  bool rc = false;

  if (_sth) {
    int tmp;
    if (!_sth->execute ()) {
      TZWARN("query failed: " << _sth->error ());
    } else if (_sth->fetch (&tmp) != ADB_OK) {
      TZWARN ("failed to fetch offset");
    } else {
      _gmtoff = tmp;
      rc = true;
    }
  }

  if (!rc) {
    TZWARN ("setting timezone to UTC due to query failure!");
    _gmtoff = 0;
  } else {
    global_gmt_offset.set (_gmtoff);
  }

  TZWARN ("ran offset compute: " << _gmtoff);

  reschedule ();
  return rc;
}


time_t 
tz_corrector_t::next_hour (time_t t)
{
  static long delay = 5;
  struct tm stm;
  long inc = 3600;
  if (!localtime_r (&t, &stm)) {
    TZWARN ("localtime failed!");
    inc = 3600;
  } else {
   
    inc = (60 - stm.tm_min) * 60 + (delay - stm.tm_sec);

    assert (inc >= 0);
    assert (inc <= 60*60 + delay);
  }
  return t + inc;
}

void
tz_corrector_t::reschedule ()
{
  time_t now = time (NULL);
  _nxt_update = next_hour (now);
}

long
tz_corrector_t::gmt_offset ()
{
  _fetching = true;
  if ((_nxt_update == 0 || okwstime() > _nxt_update) &&
      !global_gmt_offset.get (&_gmtoff, _nxt_update)) {

    run ();
  }
  _fetching = false;
  return _gmtoff;
}
