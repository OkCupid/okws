// -*-c++-*-

#ifndef __PY_RPCTYPES_H_INCLUDED__
#define __PY_RPCTYPES_H_INCLUDED__

#include <Python.h>
#include "structmember.h"

class py_rpc_base_t {
public:
  py_rpc_base_t () : _obj (NULL) {}

  py_rpc_base_t (PyObject *o) : _obj (o)
  {
    Py_XINCREF (_obj);
  }
  
  ~py_rpc_base_t ()
  {
    Py_XDECREF (_obj);
  }

  void clear ()
  {
    if (_obj) {
      Py_XDECREF (_obj);
      _obj = NULL;
    }
  }

  PyObject *obj () { return _obj; }

  bool set_obj (PyObject *o)
  {
    PyObject *tmp = _obj;
    _obj = o; // no INCREF since usually just constructed
    Py_XDECREF (tmp);
    return true;
  }

protected:
  PyObject *_obj;
};

template<size_t max = RPC_INFINITY>
class py_rpc_str : public py_rpc_base_t 
{
public:
  virtual ~py_rpc_str () {}

  char *get (size_t *sz)
  {
    char *ret;
    int i;
    if ( PyString_AsStringAndSize (_obj, &ret, &i) <= 0) {
      return NULL;
    }
    *sz = i;
    if (*sz >= max) {
      PyErr_SetString (PyExc_TypeError, 
		       "Length of string exceeded\n");
      *sz = max;
    }
    return ret;
  }

  bool set (char *buf, size_t len)
  {
    Py_XDECREF (_obj);
    if (len > max) {
      PyErr_SetString (PyExc_TypeError, 
		       "Length of string exceeded; value trunc'ed\n");
      len = max;
    }
    return (_obj = PyString_FromStringAndSize (buf, len));
  }
};

class py_u_int32_t : public py_rpc_base_t 
{
public:
  virtual ~py_u_int32_t () {}
  // XXX not sure about signed issues yet
  u_int32_t get () { return (PyInt_AsLong (_obj)); }
  bool set (u_int32_t i) {
    Py_XDECREF (_obj);
    return (_obj = PyInt_FromLong (i));
  }
};

bool rpc_traverse (XDR *xdrs, py_u_int32_t &obj);
void *py_u_int32_t_alloc ();
bool xdr_py_u_int32_t (XDR *xdrs, void *objp);

template<size_t n> inline void *
py_rpc_str_alloc ()
{
  return New py_rpc_str<n> ();
}


template<size_t n> inline bool
rpc_traverse (XDR *xdrs, py_rpc_str<n> &obj)
{
  switch (xdrs->x_op) {
  case XDR_ENCODE:
    {
      size_t sz;
      char *dat = obj.get (&sz);
      return dat && xdr_putint (xdrs, sz)
	&& xdr_putpadbytes (xdrs, dat, sz);
    }
  case XDR_DECODE:
    {
      u_int32_t size;
      if (!xdr_getint (xdrs, size) || size > n)
	return false;
      char *dp = (char *) XDR_INLINE (xdrs, size + 3 & ~3);
      if (!dp || memchr (dp, '\0', size))
	return false;
      return obj.set (dp, size);
    }
  default:
    return true;
  }
}

template<size_t n> inline bool
xdr_py_rpc_str (XDR *xdrs, void *objp)
{
  return rpc_traverse (xdrs, *static_cast<py_rpc_str<n> *> (objp));
}

#endif
