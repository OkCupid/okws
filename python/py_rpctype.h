
class py_rpc_wrap {
 public:
  PyObject *obj;
};


template<class T, size_t max>
class py_rpc_vec : public py_rpc_wrap {

};
