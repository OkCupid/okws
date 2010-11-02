// -*-c++-*-

#include "async.h"
#include "arpc.h"
#include "pub3.h"

//-----------------------------------------------------------------------

class json_XDR_dispatch_t : public v_XDR_dispatch_t {
public:
  ptr<v_XDR_t> alloc (u_int32_t rpcvers, XDR *input);
  void v_asrv_alloc (ptr<axprt> x);
  static void enable ();
};

//-----------------------------------------------------------------------

class json_fetch_constants_t 
  : public rpc_constant_collector_t {
public:
  json_fetch_constants_t ();
  void collect (const char *k, int i, rpc_constant_type_t t) ;
  void collect (const char *k, str v, rpc_constant_type_t t) {}
  void collect (const char *k, const char *c, rpc_constant_type_t t) {}
  const rpc_constant_set_t &constant_set () { return m_set; }
private:
  rpc_constant_set_t m_set;
};

//-----------------------------------------------------------------------

class json_introspection_server_t {
public:
  json_introspection_server_t (ptr<axprt> x);
  void dispatch (svccb *sbp);
  static const rpc_program s_prog;
  static bool is_associated (ptr<axprt> x);
  static const rpc_constant_set_t &constant_set ();
private:
  ptr<axprt> m_x;
  ptr<asrv> m_srv;
};

//-----------------------------------------------------------------------

class json_XDR_t : public v_XDR_t {
public:
  json_XDR_t (ptr<v_XDR_dispatch_t> d, XDR *x);
  virtual ~json_XDR_t ();
protected:
  bool is_empty () const;
  ptr<pub3::expr_t> top ();

  void freeze_error_msg (str s);
  void error_wrong_type (str s, ptr<const pub3::expr_t> x);
  void error_empty (str s);
  void error_generic (str s);

  void debug_push (str s);
  void debug_pop ();
  void push_back (ptr<pub3::expr_t> x);
  void pop_back ();
  void debug_push_slot (size_t i);

  vec<ptr<pub3::expr_t> > m_obj_stack;
  ptr<pub3::expr_t> m_root;
  vec<str> m_debug_stack;
  str m_err_msg;
};

//-----------------------------------------------------------------------

class json_decoder_t : public json_XDR_t {
public:
  json_decoder_t (ptr<v_XDR_dispatch_t> d, XDR *x);
  bool rpc_traverse (u_int32_t &obj);
  bool rpc_traverse (u_int64_t &obj);
  bool rpc_encode (str s) { return false; }
  bool rpc_decode (str *s);
  bool rpc_traverse (bigint &b);
  void enter_field (const char *f);
  void exit_field (const char *f);
  bool enter_array (u_int32_t &i, bool dyn_sized);
  void exit_array ();
  void enter_slot (size_t i);
  void exit_slot (size_t i);
  bool enter_pointer (bool &b);
  bool exit_pointer (bool b);
  bool init_decode (const char *msg, ssize_t sz);
  bool init_decode (str s);
  bool init_decode (ptr<pub3::expr_t> x);
private:
  str m_payload;
};

//-----------------------------------------------------------------------

class json_encoder_t : public json_XDR_t {
public:
  json_encoder_t (ptr<v_XDR_dispatch_t> d, XDR *x);
  bool rpc_traverse (u_int32_t &obj);
  bool rpc_traverse (u_int64_t &obj);
  bool rpc_encode (str s);
  bool rpc_decode (str *s) { return false; }
  bool rpc_traverse (bigint &b);
  void enter_field (const char *f);
  void exit_field (const char *f);
  bool enter_array (u_int32_t &i, bool dyn_sized);
  void exit_array ();
  void enter_slot (size_t i);
  void exit_slot (size_t i);
  bool enter_pointer (bool &b);
  bool exit_pointer (bool b);
  bool init_decode (const char *msg, ssize_t sz) { return false; }
  void init_encode ();
  void flush ();
protected:
  ptr<pub3::expr_list_t> top_list ();
  ptr<pub3::expr_dict_t> top_dict ();
  void push_ref (ptr<pub3::obj_ref_t> r);
  void pop_ref ();
  void set_top (ptr<pub3::expr_t> x);
private:
  ptr<pub3::obj_ref_t> m_root_ref;
  vec<ptr<pub3::obj_ref_t> > m_ref_stack;
};

//-----------------------------------------------------------------------
