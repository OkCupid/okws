
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
  void pubfiles (svccb *sbp);
  void getfile (svccb *sbp);
  void lookup (svccb *sbp);
  void config (svccb *sbp);

private:
  ptr<axprt_stream> x;
  ptr<asrv> srv;
  bool primary;
};

#endif /* _PUB_PUBD_H */
