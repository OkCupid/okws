// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Max Krohn (max@scs.cs.nyu.edu)
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

#ifndef _LIBPUB_FDFD_H
#define _LIBPUB_FDFD_H 1

//
// simple protocol for passing named File Descriptors Around
//
//  [ FLAGS | SIZE | PAYLOAD ] x fd
//
//    FLAGS - 2 byte field
//
//                   0x1 oklogd
//                   0x2 pubd
//                   0x3 x for service
//                   0x4 ctlx for service
//
//    SIZE - gives payload size -- up to 1024 bytes
//
//    PAYLOAD - (e.g., "/mysite/mail")
//

struct fdfd_msg_t {
  u_int16_t flags;
  u_int16_t size;
  char payload[PAYLOAD_SIZE];
};


class fdsource_t {
public:
  fdsource_t (int of) : outfd (of) {}
  void send (u_int64_t flags, const str &payload, cbi::ptr c);
private:
  int outfd;
  cbv::ptr eofcb;
};

#include "arpc.h"
#include "async.h"

#endif
