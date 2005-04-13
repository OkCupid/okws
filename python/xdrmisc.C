
template<class T> bool
rpc_traverse_PyString_Type (XDR *xdrs, PyObject *& obj)
{
  switch (xdrs->x_op) {
  case XDR_ENCODE: 
    {
      char *dat;
      size_t sz;
      if ( PyString_AsStringAndSize (obj, &dat, &sz) < 0) {
	return false;
      }
      return xdr_putpadbytes (xdrs, dat, sz);
    }
    break;
  case XDR_DECODE:

  }

  return false;
}
