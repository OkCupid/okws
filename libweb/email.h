// -*-c++-*-
// $Id$

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



//-----------------------------------------------------------------------------

#include "rxx.h"

//-----------------------------------------------------------------------------

class email {
private:
  str username;
  str hostname;
  bool valid;
  
public:
  email(const str& s);
  
  bool isValid() const { return valid; }
  str getUsername() const { return username; }
  str getHostname() const { return hostname; }
  
};
//-----------------------------------------------------------------------------
