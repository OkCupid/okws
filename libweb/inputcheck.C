/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
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

#include "web.h"
#include "rxx.h"


static rxx email_rxx ("[^@]+@[a-zA-Z][a-zA-Z0-9._-]*\\.[a-zA-Z]{2,5}");
static rxx zip_rxx   ("[0-9]{5}(-[0-9]{4})?");

str
check_email (const str &in)
{
  if (email_rxx.match (in))
    return in;
  else
    return NULL;
}

str 
check_zipcode (const str &in)
{
  if (zip_rxx.match (in))
    return in;
  else
    return NULL;
}
