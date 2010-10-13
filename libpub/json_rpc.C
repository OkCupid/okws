
#include "json_rpc.h"

//-----------------------------------------------------------------------

json_XDR_t::json_XDR_t (ptr<v_XDR_dispatch_t> d, XDR *x)
  : v_XDR_t (d, x) {}

//-----------------------------------------------------------------------

ptr<v_XDR_t> 
json_XDR_dispatch_t::alloc (u_int32_t rpcvers, XDR *input)
{
  ptr<v_XDR_t> ret;
  if (rpcvers == 3) {
    ret = New refcounted<json_XDR_t> (mkref (this), input);
  }
  return ret;
}

//-----------------------------------------------------------------------

bool json_XDR_t::rpc_traverse (u_int32_t &obj) { return true; }
bool json_XDR_t::rpc_traverse (u_int64_t &obj) { return true; }
bool json_XDR_t::rpc_encode (str s) { return true; }
bool json_XDR_t::rpc_decode (str *s) { return true; }
bool json_XDR_t::rpc_traverse (bigint &b) { return true; }
void json_XDR_t::enter_field (const char *f) {}
void json_XDR_t::exit_field (const char *f) {}
void json_XDR_t::enter_array (size_t i) {}
void json_XDR_t::enter_slot (size_t i) {}
void json_XDR_t::pointer (bool b) {}
void json_XDR_t::exit_array () {}

//-----------------------------------------------------------------------

void
json_XDR_t::init_decode ()
{
}


//-----------------------------------------------------------------------
