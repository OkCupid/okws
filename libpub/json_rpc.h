// -*-c++-*-

#include "async.h"
#include "arpc.h"
#include "pub3.h"

//-----------------------------------------------------------------------

class json_XDR_dispatch_t : public v_XDR_dispatch_t {
public:
  ptr<v_XDR_t> alloc (u_int32_t rpcvers, XDR *input);
};

//-----------------------------------------------------------------------

class json_XDR_t : public v_XDR_t {
public:
  json_XDR_t (ptr<v_XDR_dispatch_t> d, XDR *x);
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
  void enter_array (size_t i);
  void exit_array ();
  void enter_slot (size_t i);
  void exit_slot (size_t i);
  bool enter_pointer (bool &b);
  bool exit_pointer (bool b);
  bool init_decode (const char *msg, ssize_t sz);
  bool init_decode (str s);
  bool init_decode (ptr<pub3::expr_t> x);
private:
  ptr<pub3::expr_t> m_root;
  str m_payload;
};

//-----------------------------------------------------------------------

#if 0
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
  void enter_array (size_t i);
  void exit_array ();
  void enter_slot (size_t i);
  void pointer (bool b);
  bool init_decode (const char *msg, ssize_t sz) { return false; }
private:
  vec<ptr<pub3::obj_ref_t> > m_ref_stack;
  vec<ptr<pub3::expr_t> > m_obj_stack;
  vec<ptr<pub3::obj_ref_t> > m_ref_stack;
  ptr<pub3::expr_t> m_root;
  str m_payload;
};
#endif

//-----------------------------------------------------------------------
