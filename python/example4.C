
#include <Python.h>
#include "structmember.h"

typedef struct {
  PyObject_HEAD
  PyObject *x;   // type str
  PyObject *xx;  // type int
} foo_t;

typedef struct {
  PyObject_HEAD
  PyObject *foos;
} bar_t;

static void
foo_t_dealloc (foo_t *self)
{
  Py_XDECREF (self->x);
  Py_XDECREF (self->xx);
  self->ob_type->tp_free ((PyObject *)self);
}

static void
bar_t_dealloc (bar_t *self)
{
  Py_XDECREF (self->foos);
  self->ob_type->tp_free ((PyObject *)self);
}

static PyObject *
bar_t_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  bar_t *self;
  if (!(self = (bar_t *)type->tp_alloc (type, 0))) {
    if (!(self->foos = PyList_New (0))) {
      Py_DECREF (self);
      return NULL;
    }
  }
  return (PyObject *)self;
}

static PyObject *
foo_t_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  foo_t *self;

  self = (foo_t *)type->tp_alloc(type, 0);
  if (self != NULL) {
    if (!(self->x = PyString_FromString(""))) {
      Py_DECREF(self);
      return NULL;
    }
    self->xx = 0;
  }
  return (PyObject *)self;
}

static int
bar_t_init (bar_t *self, PyObject *args, PyObject *kwds)
{
  PyObject *x=NULL,  *tmp;

  static char *kwlist[] = {"foos", NULL};
    
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOi", kwlist, &x))
      return -1; 
    
  if (x) {
    tmp = self->foos;
    Py_INCREF(x);
    self->foos = x;
    Py_XDECREF(tmp);
  }
  return 0;
}


static int
foo_t_init(foo_t *self, PyObject *args, PyObject *kwds)
{
    PyObject *x=NULL,  *tmp;

    static char *kwlist[] = {"x", "xx", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOi", kwlist, 
				     &x, &self->xx))
      return -1; 
    
    if (x) {
      tmp = self->x;
      Py_INCREF(x);
      self->x = x;
      Py_XDECREF(tmp);
    }
    return 0;
}

static PyMemberDef foo_t_members[] = {
    {"x", T_OBJECT_EX, offsetof(foo_t, x), 0, "x field, of type string"},
    {"xx", T_INT, offsetof(foo_t, xx), 0, "xx field, of type int"},
    {NULL}  /* Sentinel */
};

static PyMemberDef bar_t_members[] = {
  { "foos", T_OBJECT_EX, offsetof (bar_t, foos), 0 "foos field, of type list"},
  {NULL}
};

static PyMethodDef foo_t_methods[] = {
  {NULL}
};

static PyMethodDef bar_t_methods[] = {
  {NULL}
};


static PyTypeObject foo_t_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "foo.foo_t",               /*tp_name*/
    sizeof(foo_t),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)foo_t_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "foo_t objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    foo_t_methods,             /* tp_methods */
    foo_t_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)foo_t_init,      /* tp_init */
    0,                         /* tp_alloc */
    foo_t_new,                 /* tp_new */
};

static PyTypeObject bar_t_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "foo.bart",               /*tp_name*/
    sizeof(bar_t),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)bar_t_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "bar_t objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    bar_t_methods,             /* tp_methods */
    bar_t_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)bar_t_init,      /* tp_init */
    0,                         /* tp_alloc */
    bar_t_new,                 /* tp_new */
};

static PyMethodDef module_methods[] = {
  {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initfoo(void) 
{
  PyObject* m;
  
  if (PyType_Ready(&foo_t_Type) < 0)
    return;
  
  m = Py_InitModule3("foo", module_methods,
		     "Example module that creates an extension type.");
  
  if (m == NULL)
    return;
  
  Py_INCREF(&foo_t_Type);
  PyModule_AddObject(m, "foo_t", (PyObject *)&foo_t_Type);
  PyModule_AddObject(m, "bar_t", (PyObject *)&bar_t_Type);
}

void *
foo_t_alloc ()
{
  return New foo_t;
}

void *
bar_t_alloc ()
{
  return New bar_t ;
}

template<class T> bool
rpc_traverse_foo_t (T &t, PyObject *obj)
{
  if (!PyObject_IsInstance (obj, (PyObject *)&foo_t_Type)) {
    warn << "object found not of type foo_t as expected!\n";
    return false;
  }
  return rpc_traverse (t, * reinterpret_cast<foo_t *> (obj));
}

template<class T> bool
rpc_traverse (T &t, foo_t &obj)
{
  return rpc_traverse_string (t, obj.x) 
    && rpc_traverse_int (t, obj.xx);
}
