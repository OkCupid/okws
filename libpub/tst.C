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

#include "pubutil.h"

struct A : public virtual refcount {
  A () {}
};

struct B : public A {

};

int
main (int argc, char *argv[])
{
  if (argc != 2)
    exit (1);

  str fn = argv[1];
  phashp_t p = file2hash (fn);
  if (!p)
    fatal << fn << ": cannot access regular file\n";
  warnx << p->to_str () << "\n";
}
