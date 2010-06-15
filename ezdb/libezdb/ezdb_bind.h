// -*-c++-*-
/* $Id: ok.h 5189 2010-03-23 21:47:11Z max $ */

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
#include "arpc.h"
#include "ezdb_prot.h"
#include "tame.h"
#include "pslave.h"
#include "ezdb_field.h"
#include "amysql.h"

namespace ezdb {

  namespace binder {

    //-----------------------------------------------------------------------

    class to_xdr_t {
    public:
      to_xdr_t ();
      to_xdr_t (u_int64_t);
      to_xdr_t (u_int32_t);
      to_xdr_t (u_int16_t);
      to_xdr_t (u_int8_t);
      to_xdr_t (bool);
      to_xdr_t (int64_t);
      to_xdr_t (int32_t);
      to_xdr_t (int16_t);
      to_xdr_t (int8_t);
      to_xdr_t (str);
      to_xdr_t (double);
      to_xdr_t (const okdate_t &d);
      const amysql_scalar_t &to_xdr () const { return _x; }
    private:
      amysql_scalar_t _x;
    };
    
    //-----------------------------------------------------------------------

    class from_xdr_typed_t {
    public:
      from_xdr_typed_t () {}
      virtual ~from_xdr_typed_t () {}
      virtual bool from_xdr (const amysql_scalar_t &x) = 0;
    };

    //-----------------------------------------------------------------------

    class from_xdr_str_t  : public from_xdr_typed_t {
    public:
      from_xdr_str_t (str *s) : _s (s) {}
      bool from_xdr (const amysql_scalar_t &x);
    private:
      str *_s;
    };

    //-----------------------------------------------------------------------

    template<class C>
    class from_xdr_num_t : public from_xdr_typed_t {
    public:
      from_xdr_num_t (C *i) : _i (i) {}
      bool from_xdr (const amysql_scalar_t &x)
      {
	bool ret = true;
	if (x.typ == AMYSQL_TYPE_INT) {
	  *_i = *x.amysql_int;
	} else if (x.typ == AMYSQL_TYPE_BOOL) {
	  *_i = *x.amysql_bool;
	} else if (x.typ == AMYSQL_TYPE_UINT64 && 
		   *x.amysql_uint64 <= u_int64_t (INT64_MAX)) {
	  *_i = *x.amysql_uint64;
	  
	} else {
	  ret = false;
	}
	return ret;
      }
    private:
      C *_i;
    };

    //-----------------------------------------------------------------------

    class from_xdr_u64_t  : public from_xdr_typed_t {
    public:
      from_xdr_u64_t (u_int64_t *u) : _u (u) {}
      bool from_xdr (const amysql_scalar_t &x);
    private:
      u_int64_t *_u;
    };
    
    //-----------------------------------------------------------------------

    class from_xdr_date_t  : public from_xdr_typed_t {
    public:
      from_xdr_date_t (okdate_t *d) : _d (d) {}
      bool from_xdr (const amysql_scalar_t &x);
    private:
      okdate_t *_d;
    };
    
    //-----------------------------------------------------------------------

    class from_xdr_double_t : public from_xdr_typed_t {
    public:
      from_xdr_double_t (double *d) : _d (d) {}
      bool from_xdr (const amysql_scalar_t &x);
    private:
      double *_d;
    };

    //-----------------------------------------------------------------------

    class from_xdr_t {
    public:
      from_xdr_t () {}
      from_xdr_t (str *s) : _p (New refcounted<from_xdr_str_t> (s)) {}
      from_xdr_t (u_int64_t *u) : _p (New refcounted<from_xdr_u64_t> (u)) {}
      from_xdr_t (u_int32_t *i) : 
	_p (New refcounted<from_xdr_num_t<u_int32_t> > (i)) {}
      from_xdr_t (u_int16_t *i) : 
	_p (New refcounted<from_xdr_num_t<u_int16_t> > (i)) {}
      from_xdr_t (u_int8_t *i) : 
	_p (New refcounted<from_xdr_num_t<u_int8_t> > (i)) {}
      from_xdr_t (int64_t *i) : 
	_p (New refcounted<from_xdr_num_t<int64_t> > (i)) {}
      from_xdr_t (int32_t *i) : 
	_p (New refcounted <from_xdr_num_t<int32_t> > (i)) {}
      from_xdr_t (int16_t *i) : 
	_p (New refcounted<from_xdr_num_t<int16_t> > (i)) {}
      from_xdr_t (int8_t *i) : 
	_p (New refcounted<from_xdr_num_t<int8_t> > (i)) {}
      from_xdr_t (bool *b) : _p (New refcounted<from_xdr_num_t<bool> > (b)) {}
      from_xdr_t (okdate_t *d) : _p (New refcounted<from_xdr_date_t> (d)) {}
      from_xdr_t (double *d) : _p (New refcounted<from_xdr_double_t> (d)) {}
      ~from_xdr_t () {}
      bool from_xdr (const amysql_scalar_t &x);
    private:
      ptr<from_xdr_typed_t> _p;
    };

    //-----------------------------------------------------------------------

    class to_xdr_bundle_t : public vec<to_xdr_t> {
    public:
      to_xdr_bundle_t (size_t s) { setsize (s); }
    };
    
    //-----------------------------------------------------------------------
    
    class from_xdr_bundle_t : public vec<from_xdr_t> {
    public:
      from_xdr_bundle_t (size_t s) { setsize (s); }
    };

    //-----------------------------------------------------------------------
  };
};
