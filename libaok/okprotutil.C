
#include "okprotutil.h"
#include "tame_profiler.h"
#include "pub3profiler.h"

#ifdef SIMPLE_PROFILER
# include "sfs_profiler.h"
#endif

//-----------------------------------------------------------------------

ok_xstatus_typ_t
ok_toggle_leak_checker (ok_diagnostic_cmd_t cmd)
{
  ok_xstatus_typ_t status;

#if defined(SIMPLE_LEAK_CHECKER) and !defined(DMALLOC)
  status = OK_STATUS_OK;
  switch (cmd) {
  case OK_DIAGNOSTIC_ENABLE:
    simple_leak_checker_enable ();
    break;
  case OK_DIAGNOSTIC_DISABLE:
    simple_leak_checker_disable ();
    break;
  case OK_DIAGNOSTIC_REPORT:
    simple_leak_checker_report ();
    break;
  case OK_DIAGNOSTIC_RESET:
    simple_leak_checker_reset ();
    break;
  default:
    status = OK_STATUS_UNKNOWN_OPTION;
    break;
  }

#else /* !SIMPLE_LEAK_CHECKER */
  status = OK_STATUS_UNAVAIL;
#endif /* SIMPLE_LEAK_CHECKER */

  return status;

}

//-----------------------------------------------------------------------

ok_xstatus_typ_t
ok_toggle_profiler (ok_diagnostic_cmd_t cmd)
{
  ok_xstatus_typ_t status;

#if defined(SIMPLE_PROFILER)
  status = OK_STATUS_OK;
  switch (cmd) {
  case OK_DIAGNOSTIC_ENABLE:
    sfs_profiler::enable ();
    break;
  case OK_DIAGNOSTIC_DISABLE:
    sfs_profiler::disable ();
    break;
  case OK_DIAGNOSTIC_REPORT:
    sfs_profiler::report ();
    break;
  case OK_DIAGNOSTIC_RESET:
    sfs_profiler::reset ();
    break;
  default:
    status = OK_STATUS_UNKNOWN_OPTION;
    break;
  }

#else /* !SIMPLE_PROFILER */
  status = OK_STATUS_UNAVAIL;
#endif /* SIMPLE_PROFILER */

  return status;

}

//-----------------------------------------------------------------------

ok_xstatus_typ_t
ok_toggle_tame_profiler (ok_diagnostic_cmd_t cmd)
{
  ok_xstatus_typ_t status;
  tame::profiler_t *p = tame::profiler_t::profiler();
  status = OK_STATUS_OK;
  switch (cmd) {
  case OK_DIAGNOSTIC_ENABLE:
    p->enable ();
    break;
  case OK_DIAGNOSTIC_DISABLE:
    p->disable ();
    break;
  case OK_DIAGNOSTIC_REPORT:
    p->report ();
    break;
  case OK_DIAGNOSTIC_RESET:
    p->clear ();
    break;
  default:
    status = OK_STATUS_UNKNOWN_OPTION;
    break;
  }

  return status;

}

//-----------------------------------------------------------------------

ok_xstatus_typ_t
ok_toggle_pub_profiler (ok_diagnostic_cmd_t cmd)
{
  ok_xstatus_typ_t status;
  pub3::profiler_t *p = pub3::profiler_t::profiler();
  status = OK_STATUS_OK;
  switch (cmd) {
  case OK_DIAGNOSTIC_ENABLE:
    if (!p->enable ()) { status = OK_STATUS_UNAVAIL; }
    break;
  case OK_DIAGNOSTIC_DISABLE:
    p->disable ();
    break;
  case OK_DIAGNOSTIC_REPORT:
    p->report ();
    break;
  case OK_DIAGNOSTIC_RESET:
    p->reset ();
    break;
  default:
    status = OK_STATUS_UNKNOWN_OPTION;
    break;
  }

  return status;

}

//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
