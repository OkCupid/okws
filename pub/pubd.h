
// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Max
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#ifndef _PUB_PUBD_H
#define _PUB_PUBD_H

#include "arpc.h"
#include "pub.h"
#include "xpub.h"

class pubserv_t {
public:
  pubserv_t (ptr<axprt_stream> x, bool p = false);
  void dispatch (svccb *sbp);
  void getfile (svccb *sbp);
  void lookup (svccb *sbp);
  void config (svccb *sbp);

  void pubfiles (svccb *sbp);
  void pubfiles (svccb *sbp, xpub_result_t *pres);

  // next functions for the new publishing system

  /**
   * call pubfiles2 to start a "session" for publishing a fileset.
   * once the session has been successfully started, pubd will return
   * a cookie that corresponds to the pubd-side state for the fileset
   * in question.  Using the cookie as a handle, the client can fetch
   * one file at a time from pubd.
   */
  void pubfiles2 (svccb *sbp);

  /**
   * Given a cookie and a filenumber, return the corresponding file.
   */
  void pubfiles2_getfile (svccb *sbp);

  /**
   * close the session given by the supplied cookie. this will delete
   * all pubd-side state corresponding to this session.
   */
  void pubfiles2_close (svccb *sbp);

private:
  ptr<axprt_stream> x;
  ptr<asrv> srv;
  bool primary;

  xpub_cookie_t next_session_cookie () { return ++this_cookie; }
  qhash<xpub_cookie_t, ptr<xpub_result_t> > sessions;
  xpub_cookie_t this_cookie;
};

#endif /* _PUB_PUBD_H */
