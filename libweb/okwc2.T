// -*-c++-*-

#include "okwc2.h"

TAMED void
okwc2_t::req_T (ptr<okwc2_req_t> req, okwc2_cb_t cb)
{
  VARS {
    int status (0);
    ptr<okwc2_resp_t> resp;
    ptr<okwc2_t> hold;
    ptr<hostent> he;
    int dns_err;
    coordgroup_t<bool> G;
    bool cancelled;
    int fd;
  }
  
  // Keep this object from being freed from underneath us.
  hold = mkref (this);

  resp->notify_on_cancel (@[G, true]());
  do_dns_request (_hostname, @[G, false](he, dns_err)); 
  if (cancellable_wait (G, cb)) return;

  if (dns_err || !he) {
    status = HTTP_CONNECTION_FAILED;
  } else {
    tcpconnect (*(in_addr *)he->h_addr, _port, @[G, false](fd));
    if (cancellable_wait (G, cb)) return;

  }

  SIGNAL (cb, status, resp);
}
