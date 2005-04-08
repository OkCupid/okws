
#include <Python.h>
#include <stdlib.h>
#include "async.h"

static PyObject *
Example_amain (PyObject *self, PyObject *args)
{
  // of course, does not return
  amain ();

  Py_INCREF (Py_None);
  return Py_None;
}

static void
Example_delaycb_cb (PyObject *handler)
{
  PyObject *args, *pres;
  args = Py_BuildValue ("");
  pres = PyEval_CallObject (handler, args);
  Py_DECREF (handler);
  Py_DECREF (args);
  if (pres != NULL) {
    Py_DECREF (pres);
  }
}


static PyObject *
Example_delaycb (PyObject *self, PyObject *args)
{
  PyObject *handler;
  int d;
  PyArg_Parse (args, "(iO)", &d, &handler);
  delaycb  (d, 0, wrap (Example_delaycb_cb, handler));
  Py_INCREF (Py_None);
  return Py_None;
}

static struct PyMethodDef example_methods[] = {
  { "amain",  Example_amain },
  { "delaycb", Example_delaycb },
  { NULL, NULL }
};

void initexample()
{
  (void) Py_InitModule ("example1", example_methods);
}
