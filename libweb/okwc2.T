// -*-c++-*-

#include "okwc2.h"

TAMED static void
cancellable_wait (coordgroup_t<bool> G, okwc2_cb_t cb, cbb callercv)
{
  VARS {
    bool cancelled;
    bool dummy;
  }
  WAIT (G, cancelled);
  if (cancelled) {
    SIGNAL (cb, HTTP_TIMEOUT, NULL);
  }
  SIGNAL (callercv, cancelled);
  if (cancelled) {
    WAIT (G, dummy);
  }
}

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
    int fd;
    bool cancelled;
    ptr<ahttpcon> x;
  }
  
  // Keep this object from being freed from underneath us.
  hold = mkref (this);

  req->notify_on_cancel (@[G, true]());
  do_dns_request (_hostname, @[G, false](he, dns_err)); 

  BLOCK { cancellable_wait (G, cb, @(cancelled)); }
  if (cancelled) return;

  if (dns_err || !he) {
    status = HTTP_CONNECTION_FAILED;
  } else {
    tcpconnect (*(in_addr *)he->h_addr, _port, @[G, false](fd));

    BLOCK { cancellable_wait (G, cb, @(cancelled)); }
    if (cancelled) return;

    if (fd < 0) {
      status = HTTP_CONNECTION_FAILED;
    } else {
      x = ahttpcon::alloc (fd);
      req->make (x, @[G,false](status)); 

      BLOCK { cancellable_wait (G, cb, @(cancelled)); }
      if (cancelled) return;

      if (status == HTTP_OK) {
	get_response (req, x, @[G,false](status, resp));

	BLOCK { cancellable_wait (G, cb, @(cancelled)); }
	if (cancelled) return;
      }
    }
  }
  // remove the canceller
  req->too_late_to_cancel ();
  G.remove_join ();
  SIGNAL (cb, status, resp);
}
