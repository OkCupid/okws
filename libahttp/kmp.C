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

#include "kmp.h"

void
kmp_matcher_t::preproc ()
{
  memset (_pi, 0, _len * sizeof (u_int));
  u_int k = 0;
  for (u_int q_tmp = 1; q_tmp < _len; q_tmp++) {
    while (k > 0  && _PP[k] != _PP[q_tmp]) 
      k = _pi[k-1];
    if (_PP[k] == _PP[q_tmp])
      k++;
    _pi[q_tmp] = k;
  }
}
