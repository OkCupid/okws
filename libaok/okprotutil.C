
#include "okprotutil.h"

//-----------------------------------------------------------------------

ok_xstatus_typ_t
ok_toggle_leak_checker (ok_leak_checker_cmd_t cmd)
{
  ok_xstatus_typ_t status;

#if defined(SIMPLE_LEAK_CHECKER) and !defined(DMALLOC)
  status = OK_STATUS_OK;
  switch (cmd) {
  case OK_LEAK_CHECKER_ENABLE:
    simple_leak_checker_enable ();
    break;
  case OK_LEAK_CHECKER_DISABLE:
    simple_leak_checker_disable ();
    break;
  case OK_LEAK_CHECKER_REPORT:
    simple_leak_checker_report ();
    break;
  case OK_LEAK_CHECKER_RESET:
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
