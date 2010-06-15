/* $Id: pt1d.C 1007 2005-09-11 21:45:33Z max $ */

/*
 *
 * Copyright (C) 2003-4 by Maxwell Krohn (max@okcupid.com)
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
#include "arpc.h"
#include "ezdb_prot.h"
#include "pslave.h"

#include "okwsconf.h"
#include "amysql.h"
#include "mystmt.h"
#include "web.h"
#include "crypt.h"

namespace ezdb {

  //-----------------------------------------------------------------------
  
  struct field_t {
    field_t (const MYSQL_FIELD &f);
    field_t (const amysql_field_t &x);
    void to_xdr (amysql_field_t *x) const;
    
    str name () const { return _name; }
    str org_name () const { return _org_name; }
    str table () const { return _table; }
    str org_table () const { return _org_table; }
    str db () const { return _db; }
    str catalog () const { return _catalog; }
    size_t len () const { return _length; }
    size_t max_len () const { return _max_length; }
    int flags () const { return _flags; }
    int decimals () const { return _decimals; }
    int charsetnr () const { return _charsetnr; }
    eft_t type () const { return _type; }
    
    str _name, _org_name, _table, _org_table, _db, _catalog, _def;
    size_t _length, _max_length;
    int _flags, _decimals, _charsetnr;
    eft_t _type;
  };

  //---------------------------------------------------------------------

  class fields_t : public vec<field_t> {
  public:
    fields_t () {}
    void init (const amysql_fields_t &x);
  };

};

//-----------------------------------------------------------------------

