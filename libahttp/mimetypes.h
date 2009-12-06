// -*-c++-*-
/* $Id: pub.h 1760 2006-05-20 02:53:00Z max $ */

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

#ifndef _LIBAHTTP_MIMETYPES_H
#define _LIBAHTTP_MIMETYPES_H 1

#include "async.h"
#include "rxx.h"
#include "pub.h"
#include "pub3obj.h"

class mime_type_map_t {
public:
  mime_type_map_t (const pub3::obj_t &in);
  str lookup (const str &in, str *sffx = NULL) const;
protected:
  void add (str k, str v);
  qhash<str,str> _table;
  vec<str> _suffixes;
  rxx *_matcher;
};


#endif /* _LIBAHTTP_MIMETYPES_H */
