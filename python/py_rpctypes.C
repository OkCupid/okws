
#include "py_rpctypes.h"

bool
rpc_traverse (XDR *xdrs, u_int32_t &obj)
{
  switch (xdrs->x_op) {
  case XDR_ENCODE:
    return xdr_putint (xdrs, obj);
  case XDR_DECODE:
    return xdr_getint (xdrs, obj);
  default:
    return true;
  }
}

void *
py_u_int32_t_alloc ()
{
  return New py_u_int32_t ();
}

bool
xdr_py_u_int32_t (XDR *xdr, void *objp)
{
  return rpc_traverse (xdrs, *static_cast<u_int32_t *> (objp));
}

