
#include "ezdb.h"
#include "ezdb_bind.h"
#include "web_prot.h"

namespace ezdb {
  namespace binder {
    //-----------------------------------------------------------------------

    to_xdr_t::to_xdr_t () : _x (AMYSQL_TYPE_NULL) {}
    to_xdr_t::to_xdr_t (u_int64_t i) : _x (AMYSQL_TYPE_UINT64) 
    { *_x.amysql_uint64 = i; }
    to_xdr_t::to_xdr_t (int64_t i) : _x (AMYSQL_TYPE_INT) 
    { *_x.amysql_int = i; }

    to_xdr_t::to_xdr_t (u_int32_t i) 
    {
      if (i > u_int32_t (INT32_MAX)) {
	_x.set_typ (AMYSQL_TYPE_UINT64);
	*_x.amysql_uint64 = i;
      } else {
	_x.set_typ (AMYSQL_TYPE_INT);
	*_x.amysql_int = i;
      }
    }

    to_xdr_t::to_xdr_t (int32_t i) : _x (AMYSQL_TYPE_INT) 
    { *_x.amysql_int = i; }
    to_xdr_t::to_xdr_t (u_int16_t i) : _x (AMYSQL_TYPE_INT) 
    { *_x.amysql_int = i; }
    to_xdr_t::to_xdr_t (int16_t i) : _x (AMYSQL_TYPE_INT) 
    { *_x.amysql_int = i; }
    to_xdr_t::to_xdr_t (u_int8_t i) : _x (AMYSQL_TYPE_INT) 
    { *_x.amysql_int = i; }
    to_xdr_t::to_xdr_t (int8_t i) : _x (AMYSQL_TYPE_INT) { *_x.amysql_int = i; }
    to_xdr_t::to_xdr_t (bool b) : _x (AMYSQL_TYPE_BOOL) { *_x.amysql_bool = b; }
    to_xdr_t::to_xdr_t (str s) : _x (AMYSQL_TYPE_STRING) 
    { *_x.amysql_string = s; }

    to_xdr_t::to_xdr_t (double d) : _x (AMYSQL_TYPE_DOUBLE)
    {   
      strbuf b;
      b << d;
      *_x.amysql_double = b;
    }

    //-----------------------------------------------------------------------

    to_xdr_t::to_xdr_t (const okdate_t &d)  : _x (AMYSQL_TYPE_DATE) { 
      d.to_xdr (_x.amysql_date); 
    }

    //-----------------------------------------------------------------------

    bool
    from_xdr_str_t::from_xdr (const amysql_scalar_t &x)
    {
      bool ret = true;
      strbuf b;
      switch (x.typ) {
      case AMYSQL_TYPE_INT:
	b << *x.amysql_int;
	break;
      case AMYSQL_TYPE_UINT64:
	b << *x.amysql_uint64;
	break;
      case AMYSQL_TYPE_BOOL:
	b << *x.amysql_bool;
	break;
      case AMYSQL_TYPE_STRING:
	*_s = *x.amysql_string;
	break;
      case AMYSQL_TYPE_OPAQUE:
	b.buf (x.amysql_opaque->base (), x.amysql_opaque->size ());
	break;
      case AMYSQL_TYPE_DATE:
	{
	  okdate_t d (*x.amysql_date);
	  *_s = d.to_str ();
	}
	break;
      default:
	ret = false;
	break;
      }
      if (ret && b.len ()) { *_s = b; }
      return ret;
    }

    //-----------------------------------------------------------------------

    bool
    from_xdr_date_t::from_xdr (const amysql_scalar_t &x)
    {
      bool ret = false;
      switch (x.typ) {
      case AMYSQL_TYPE_INT:
	_d->set (*x.amysql_int);
	break;
      case AMYSQL_TYPE_UINT64:
	_d->set (*x.amysql_uint64);
	break;
      case AMYSQL_TYPE_DATE:
	_d->set (*x.amysql_date);
	break;
      default:
	ret = false;
	break;
      }
      return ret;
    }

    //-----------------------------------------------------------------------

    bool
    from_xdr_u64_t::from_xdr (const amysql_scalar_t &x)
    {
      bool ret = true;
      switch (x.typ) {
      case AMYSQL_TYPE_INT:
	if (*x.amysql_int >= 0) {
	  *_u = *x.amysql_int;
	} else {
	  ret = false;
	}
	break;
      case AMYSQL_TYPE_UINT64:
	*_u = *x.amysql_uint64;
	break;
      default:
	ret = false;
	break;
      }
      return ret;
    }

    //-----------------------------------------------------------------------

    bool
    from_xdr_double_t::from_xdr (const amysql_scalar_t &x)
    {
      bool ret = true;
      switch (x.typ) {
      case AMYSQL_TYPE_INT:
	*_d = *x.amysql_int;
	break;
      case AMYSQL_TYPE_UINT64:
	*_d = *x.amysql_uint64;
	break;
      case AMYSQL_TYPE_BOOL:
	*_d = *x.amysql_bool;
	break;
      case AMYSQL_TYPE_DOUBLE:
	ret = convertdouble (*x.amysql_double, _d);
	break;
      default:
	ret = false;
      }
      return ret;
    }

    //-----------------------------------------------------------------------

    bool
    from_xdr_t::from_xdr (const amysql_scalar_t &x)
    {
      return _p ? _p->from_xdr (x) : false;
    }

    //-----------------------------------------------------------------------

  };
};
