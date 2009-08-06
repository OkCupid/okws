// -*-c++-*-
/* $Id: pubutil.h 4155 2009-02-27 14:58:37Z max $ */


/*
 *
 * Copyright (C) 2003 Maxwell Krohn (max@okcupid.com)
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

#ifndef _LIBPUB_PESCAPE_H 
#define _LIBPUB_PESCAPE_H 1

#include "async.h"
#include "qhash.h"

str json_escape (const str &s, bool qs);
str xss_escape (const char *s, size_t l);
str xss_escape (const str &s);
str filter_tags (const str &in, const bhash<str> &exceptions);


#endif /* _LIBPUB_PESCAPEL_H */
