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

#ifndef _LIBPUB_AUTHTOK_H 
#define _LIBPUB_AUTHTOK_H 1

#include "amisc.h"
#include "str.h"
#include "async.h"
#include "ihash.h"
#include "xpub.h"

//
// XXX - install script should auto-generate these based upon some
//       random data.  or... okws should come with a support script
//       to run when installing on a new site, for active developers
//       on the core
//
#define OKLOGD_AUTHTOK "uuq9182jxoeroiuasdfqppiourewqlkjhasfdbzxuiyasdf"
#define PUBD_AUTHTOK   "234908ew2#$S981#E$SDf908zlhe[qwer{}2#$asdf09341a"

ptr<okauthtok_t> authtok_alloc (const str &s);
bool authtok_cmp (const okauthtok_t &t1, const okauthtok_t &t2);

#endif /* _LIBPUB_AUTHTOK_H */
