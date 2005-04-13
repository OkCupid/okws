
template<class T>
class PyObjTyped {
public:
  PyObjTyped (PyObject *o) : obj (o) { Py_INCREF (o); }
  ~PyObjTyped () { Py_DECREF (obj); }

  T *cast () { return reinterpret_cast<T *> (obj); }
  PyObject *obj;
};

PyObjTyped<rpc_str <n>>
PyObjTyped<u_int32_t>
PyObjTyped<rpc_vec <foo_t, 10> > 


template<class T, size_t max>
class py_rpc_vec : public py_rpc_wrap {

};

struct foo_t {
  py_rpc_str<100>              x;
  py_u_int32_t                 xx;
  py_rpc_vec<u_int32_t, 100>   xxx;
};
