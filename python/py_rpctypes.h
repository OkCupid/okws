// -*-c++-*-

#infdef __PY_RPCTYPES_H_INCLUDED__
#define __PY_RPCTYPES_H_INCLUDED__

#include <Python.h>
#include "structmember.h"

class py_rpc_base_t {
public:
  py_rpc_base_t (Py_Object *o) : _obj (o)
  {
    Py_INCREF (o);
  }
  
  ~py_rpc_base_t ()
  {
    Py_DECREF (o);
  }

  PyObject *obj () { return _obj; }

private:
  PyObject *_obj;
};

template<size_t max = RPC_INFINITY>
class py_rpc_str_t : public py_rpc_base_t 
{
  virtual ~py_rpc_str_t () {}

  char *get (size_t *sz)
  {
    char *ret;
    if ( PyString_AsStringAndSize (_obj, &ret, sz) != 0) {
      return NULL;
    }
    return ret;
  }


};


#endif
