// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
 *                         Patrick Crosby (patrick@okcupid.com)
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

#include "email.h"

static rxx 
valid_pattern("^(([a-zA-Z0-9_\\.\\-])+)\\@((([a-zA-Z0-9\\-])+\\.)+"
	      "([a-zA-Z0-9]{2,4})+)$", "S");

email::email(const str& s) : username(""), hostname(""), valid(false)
{
  if (s) {
    valid = valid_pattern.match(s);
  }
  if (valid) {
    if (valid_pattern[1]) {
      username = valid_pattern[1];
    }
    if (valid_pattern[3]) {
      hostname = valid_pattern[3];
    }
  }
}
