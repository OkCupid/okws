// -*-c++-*-
/* $Id: web.h 5098 2010-01-21 02:51:43Z max $ */

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

#pragma once
#include "async.h"

//=======================================================================

class global_gmt_offset_t {
public:
  global_gmt_offset_t () : _when_updated (0), _val (0) {}
  bool get (long *val, time_t freshness = 0);
  long get () { return _val; }
  void set (long v);
private:
  time_t _when_updated;
  long _val;
};

//=======================================================================

extern global_gmt_offset_t global_gmt_offset;

//=======================================================================

