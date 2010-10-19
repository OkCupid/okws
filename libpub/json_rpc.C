
#include "bigint.h"
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
    switch (input->x_op) {
    case XDR_DECODE:
      ret = New refcounted<json_decoder_t> (mkref (this), input);
      break;
      /*
       * not ready for this yet...
       case XDR_ENCODE:
       ret = New refcounted<json_encoder_t> (mkref (this), input);
       break;
      */
    default:
      break;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

json_decoder_t::json_decoder_t (ptr<v_XDR_dispatch_t> d, XDR *x)
  : json_XDR_t (d, x) {}

//-----------------------------------------------------------------------

bool 
json_decoder_t::rpc_traverse (u_int32_t &obj) 
{ 
  int64_t tmp;
  bool ret = false;
  if (is_empty ()) { error_empty ("int"); }
  else if (!top ()->to_int (&tmp)) { error_wrong_type ("int", top ()); }
  else if (tmp > int64_t (UINT32_MAX) || tmp < 0) { 
    error_generic (strbuf ("u_int32_t is out of range (value was %" 
			   PRId64 ")", tmp)); 
  } else {
    ret = true;
    obj = tmp;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool 
json_decoder_t::rpc_traverse (u_int64_t &obj) 
{ 
  u_int64_t tmp;
  bool ret = false;
  if (is_empty ()) { error_empty ("uint"); }
  else if (!top ()->to_uint (&tmp)) { error_wrong_type ("uint", top ()); }
  else {
    ret = true;
    obj = tmp;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
json_decoder_t::rpc_decode (str *s)
{
  bool ret = false;
  if (is_empty ()) { error_empty ("string"); }
  else if (!(*s = top()->to_str (false))) { 
    error_wrong_type ("string", top()); 
  } else {
    ret = true;
  }
  return ret;
}

//-----------------------------------------------------------------------

void 
json_decoder_t::enter_field (const char *f) 
{
  debug_push (f);
  ptr<pub3::expr_dict_t> d;
  ptr<pub3::expr_t> x;
  if (is_empty ()) { error_empty ("dictionary"); }
  else if (!(d = top ()->to_dict ())) {
    error_wrong_type ("dictionary", top ());
  } else if (!(x = d->lookup (f))) {
    error_generic (strbuf ("cannot find field '%s'", f));
  }
  push_back (x);
}

//-----------------------------------------------------------------------

void
json_decoder_t::exit_field (const char *f)
{
  pop_back ();
  debug_pop ();
}

//-----------------------------------------------------------------------

void
json_decoder_t::enter_slot (size_t i)
{
  debug_push (strbuf ("[%zu]", i));
  ptr<pub3::expr_list_t> l;
  ptr<pub3::expr_t> x;
  if (is_empty ()) { error_empty ("array"); }
  else if (!(l = top ()->to_list ())) {
    error_wrong_type ("array", top ());
  } else if (i >= l->size ()) {
    error_generic (strbuf ("array slot %zu is out of bound\n", i));
  } else {
    x = (*l)[i];
  }
  push_back (x);
}

//-----------------------------------------------------------------------

void
json_decoder_t::exit_slot (size_t i)
{
  pop_back ();
  debug_pop ();
}

//-----------------------------------------------------------------------

bool 
json_decoder_t::rpc_traverse (bigint &b) 
{ 
  bool ret = false;
  str tmp;
  if (is_empty ()) { error_empty ("bigint"); }
  else if (!(tmp = top()->to_str (false))) { 
    error_wrong_type ("string", top()); 
  } else {
    b = tmp;
    ret = true;
  }
  return ret;
}

//-----------------------------------------------------------------------

void json_decoder_t::enter_array (size_t i) {}
void json_decoder_t::exit_array () {}

//-----------------------------------------------------------------------

bool
json_decoder_t::enter_pointer (bool &nonnil) 
{
  ptr<pub3::expr_list_t> l;
  ptr<pub3::expr_t> x;

  bool ret = false;
  if (is_empty ()) { error_empty ("array/pointer"); }
  else if (!(l = top()->to_list ())) {
    error_wrong_type ("array/pointer", top());
  } else if (l->size () > 0) {
    nonnil = true;
    x = (*l)[0];
    ret = true;
  } else {
    ret = true;
    nonnil = false;
  }
  push_back (x);
  return ret;
}

//-----------------------------------------------------------------------

bool
json_decoder_t::exit_pointer (bool nonnil)
{
  pop_back ();
  return true;
}

//-----------------------------------------------------------------------

bool
json_decoder_t::init_decode (const char *msg, ssize_t sz)
{
  return (sz > 0 && init_decode (str (msg, sz)));
}

//-----------------------------------------------------------------------

bool
json_decoder_t::init_decode (str s)
{
  return ((m_payload = s) && 
	  (m_root = pub3::json_parser_t::parse (s)));
}

//-----------------------------------------------------------------------

bool
json_decoder_t::init_decode (ptr<pub3::expr_t> x)
{
  if (x) { 
    m_root = x;
    m_obj_stack.push_back (x);
  }
  return x;
}

//-----------------------------------------------------------------------

bool
json_XDR_t::is_empty () const
{
  return !m_obj_stack.size () || 
    !m_obj_stack.back () || 
    m_obj_stack.back()->is_null ();
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
json_XDR_t::top ()
{
  ptr<pub3::expr_t> ret;
  if (m_obj_stack.size ()) { ret = m_obj_stack.back (); }
  return ret;
}

//-----------------------------------------------------------------------

void json_XDR_t::pop_back () { m_obj_stack.pop_back (); }
void json_XDR_t::push_back (ptr<pub3::expr_t> x) { m_obj_stack.push_back (x); }

//-----------------------------------------------------------------------

void json_XDR_t::debug_push (str s) { m_debug_stack.push_back (s); }
void json_XDR_t::debug_pop () { m_debug_stack.pop_back(); }

//-----------------------------------------------------------------------

void
json_XDR_t::error_generic (str s)
{
  strbuf b;
  b << "at field: ";
  size_t n = m_debug_stack.size ();
  for (size_t i = 0; i < n; i++) {
    str el = m_debug_stack[i];
    if (i != 0 && el[0] != '[') { b << "."; }
    b << el;
  }
  b << ": " << s;
  m_err_msg = b;
}

//-----------------------------------------------------------------------

void
json_XDR_t::error_empty (str s)
{
  error_generic (strbuf ("empty field; expected type %s", s.cstr ()));
}

//-----------------------------------------------------------------------

void
json_XDR_t::error_wrong_type (str s, ptr<const pub3::expr_t> x)
{
  str got = x->type_to_str ();
  strbuf msg ("got type %s; expected %s", got.cstr (), s.cstr ());
  error_generic (msg);
}

//-----------------------------------------------------------------------
