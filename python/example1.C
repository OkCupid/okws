
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
  printf ("hit CB, with arg=%p\n", handler);
  PyObject *pres;
  pres = PyEval_CallObject (handler, NULL);
  printf ("past call object...\n");
  Py_XDECREF (handler);
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
  Py_XINCREF (handler);
  printf ("delay CB with args: %d, %p, %p\n", d, self, handler);
  delaycb  (d, 0, wrap (Example_delaycb_cb, handler));
  Py_INCREF (Py_None);
  return Py_None;
}

static struct PyMethodDef example1_methods[] = {
  { "amain",  Example_amain },
  { "delaycb", Example_delaycb },
  { NULL, NULL }
};

extern "C" {
void initexample1()
{
  (void) Py_InitModule ("example1", example1_methods);
}
}
