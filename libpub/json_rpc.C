
#include "bigint.h"
#include "json_rpc.h"

//-----------------------------------------------------------------------

json_XDR_t::json_XDR_t (ptr<v_XDR_dispatch_t> d, XDR *x)
  : v_XDR_t (d, x) {}

//-----------------------------------------------------------------------

json_XDR_dispatch_t::json_XDR_dispatch_t () {}

//-----------------------------------------------------------------------

ptr<v_XDR_t> 
json_XDR_dispatch_t::alloc (u_int32_t rpcvers, XDR *input)
{
  ptr<v_XDR_t> ret;
  if (rpcvers == JSON_RPC_VERS) {
    switch (input->x_op) {
    case XDR_DECODE:
      ret = alloc_decoder (input);
      break;
    case XDR_ENCODE:
      ret = alloc_encoder (input);
      break;
    default:
      break;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<json_decoder_t> 
json_XDR_dispatch_t::alloc_decoder (XDR *input)
{
  return New refcounted<json_decoder_t> (mkref (this), input);
}

//-----------------------------------------------------------------------

ptr<json_encoder_t> 
json_XDR_dispatch_t::alloc_encoder (XDR *input)
{
  return New refcounted<json_encoder_t> (mkref (this), input);
}

//-----------------------------------------------------------------------

ptr<json_XDR_dispatch_t> json_XDR_dispatch_t::s_obj;

//-----------------------------------------------------------------------

ptr<json_XDR_dispatch_t> 
json_XDR_dispatch_t::get_singleton_obj ()
{
  if (!s_obj) {
    s_obj = New refcounted<json_XDR_dispatch_t> ();
  }
  return s_obj;
}

//-----------------------------------------------------------------------

void
json_XDR_dispatch_t::enable ()
{
  v_XDR_dispatch = get_singleton_obj ();
}

//-----------------------------------------------------------------------

bool
json_XDR_dispatch_t::is_enabled ()
{
  return v_XDR_dispatch && (v_XDR_dispatch == s_obj);
}

//-----------------------------------------------------------------------

json_decoder_t::json_decoder_t (ptr<v_XDR_dispatch_t> d, XDR *x)
  : json_XDR_t (d, x) {}

//-----------------------------------------------------------------------

bool 
json_decoder_t::rpc_traverse (int32_t &obj) 
{ 
  int64_t tmp;
  bool ret = false;
  if (is_empty ()) { error_empty ("int"); }
  else if (!top ()->to_int (&tmp)) { error_wrong_type ("int", top ()); }
  else if (tmp > int64_t (INT32_MAX) || tmp < int64_t(INT32_MIN)) { 
    error_generic (strbuf ("int32_t is out of range (value was %" 
			   PRId64 ")", tmp)); 
  } else {
    ret = true;
    obj = tmp;
  }
  return ret;
}

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
json_decoder_t::rpc_traverse (int64_t &obj) 
{ 
  int64_t tmp;
  bool ret = false;
  if (is_empty ()) { error_empty ("int"); }
  else if (!top ()->to_int (&tmp)) { error_wrong_type ("int", top ()); }
  else {
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
json_decoder_t::rpc_traverse (double &obj) 
{ 
  double tmp;
  bool ret = false;
  if (is_empty ()) { error_empty ("double"); }
  else if (!top ()->to_double (&tmp)) { error_wrong_type ("double", top ()); }
  else {
    ret = true;
    obj = tmp;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
json_decoder_t::rpc_decode_opaque (str *s)
{
  str tmp;
  bool ret = rpc_decode (&tmp);
  if (ret && !(*s = dearmor64 (tmp))) {
    error_generic ("failed to base-64 decode input string");
    ret = false; 
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
json_encoder_t::rpc_encode_opaque (str s)
{
  str tmp = armor64 (s);
  return rpc_encode (tmp);
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
  // top level!
  if (!f) { return; }

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
  // top level!
  if (!f) { return; }
  pop_back ();
  debug_pop ();
}

//-----------------------------------------------------------------------

void
json_decoder_t::enter_slot (size_t i)
{
  debug_push_slot (i);
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
    b = bool(tmp);
    ret = true;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool 
json_decoder_t::enter_array (u_int32_t &i, bool dyn_sized) 
{ 
  bool ret = false;
  ptr<pub3::expr_list_t> l;
  if (is_empty ()) { error_empty ("array"); }
  else if (!(l = top ()->to_list ())) {
    error_wrong_type ("array", top ());
  } else {
    ret = true;
    i = l->size ();
  }
  return ret; 
}
//-----------------------------------------------------------------------

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
  // Remove any padding of NULL bytes at the end of the string
  ssize_t len = 0;
  while (msg[len] != 0 && len < sz) { len++; }

  return (sz > 0 && init_decode (str (msg, len)));
}

//-----------------------------------------------------------------------

bool
json_decoder_t::init_decode (str s)
{
  ptr<pub3::expr_t> x;
  bool ret = false;
  m_payload = s;
  ptr<pub3::json_parser_t> p = New refcounted<pub3::json_parser_t> ();
  
  if (!s) { 
    warn << "json-rpc: decode of empty string failed\n";
  } else if (!(x = p->mparse (s))) {
    warn << "json-rpc: decode of packet failed: " << s << "\n";
    const vec<str> &e = p->get_errors ();
    for (size_t i = 0; i < e.size (); i++) {
      warn << "json-rpc: error: " << e[i] << "\n";
    } 
  } else {
    ret = init_decode (x); 
  }
  return ret;
}

//-----------------------------------------------------------------------

bool json_decoder_t::rpc_traverse_null () { return true; }

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
  b << "at field ";
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

json_encoder_t::json_encoder_t (ptr<v_XDR_dispatch_t> d, XDR *x)
  : json_XDR_t (d, x) 
{
  m_root_ref = pub3::plain_obj_ref_t::alloc ();
  push_ref (m_root_ref);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
json_encoder_t::root_obj ()
{
  ptr<pub3::expr_t> x = m_root_ref->get ();
  return x;
}

//-----------------------------------------------------------------------

bool
json_encoder_t::rpc_traverse (int32_t &obj)
{
  set_top (pub3::expr_int_t::alloc (obj));
  return true;
}

//-----------------------------------------------------------------------

bool
json_encoder_t::rpc_traverse (u_int32_t &obj)
{
  set_top (pub3::expr_int_t::alloc (obj));
  return true;
}

//-----------------------------------------------------------------------

bool
json_encoder_t::rpc_traverse (int64_t &obj)
{
  set_top (pub3::expr_int_t::alloc (obj));
  return true;
}

//-----------------------------------------------------------------------

bool
json_encoder_t::rpc_traverse (u_int64_t &obj)
{
  set_top (pub3::expr_uint_t::alloc (obj));
  return true;
}

//-----------------------------------------------------------------------

bool
json_encoder_t::rpc_traverse (double &obj)
{
  set_top (pub3::expr_double_t::alloc (obj));
  return true;
}

//-----------------------------------------------------------------------

bool
json_encoder_t::rpc_traverse_null()
{
  set_top (pub3::expr_null_t::alloc ());
  return true;
}

//-----------------------------------------------------------------------

bool
json_encoder_t::rpc_traverse (bigint &b)
{
  str s = b.getstr (10);
  set_top (pub3::expr_str_t::alloc (s));
  return true;
}
 
//-----------------------------------------------------------------------

void
json_encoder_t::enter_field (const char *field)
{
  if (field) {
    debug_push (field);
    push_ref (pub3::obj_ref_dict_t::alloc (top_dict (), field));
  }
}

//-----------------------------------------------------------------------

void
json_encoder_t::exit_field (const char *f)
{
  if (f) { pop_ref (); }
}

//-----------------------------------------------------------------------

void
json_encoder_t::enter_slot (size_t i)
{
  debug_push_slot (i);
  push_ref (pub3::obj_ref_list_t::alloc (top_list(), i));
}

//-----------------------------------------------------------------------

void
json_encoder_t::exit_slot (size_t i)
{
  debug_pop ();
  pop_ref ();
}

//-----------------------------------------------------------------------

bool
json_encoder_t::rpc_encode (str s)
{
  set_top (pub3::expr_str_t::alloc (s));
  return true;
}

//-----------------------------------------------------------------------

void json_encoder_t::exit_array () {}

//-----------------------------------------------------------------------

bool
json_encoder_t::enter_array (u_int32_t &i, bool dyn_sized)
{
  ptr<pub3::expr_list_t> l = pub3::expr_list_t::alloc ();
  set_top (l);
  return true;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_list_t>
json_encoder_t::top_list ()
{
  ptr<pub3::expr_list_t> ret;
  assert (m_obj_stack.size ());
  ret = m_obj_stack.back ()->to_list ();
  assert (ret);
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_dict_t>
json_encoder_t::top_dict ()
{
  ptr<pub3::expr_dict_t> ret;
  ptr<pub3::expr_t> &back = m_obj_stack.back ();
  if (!back) {
    ret = pub3::expr_dict_t::alloc ();
    back = ret;
    set_top (ret);
  } else {
    ret = back->to_dict ();
    assert (ret);
  }
  return ret;
}

//-----------------------------------------------------------------------

void
json_XDR_t::debug_push_slot (size_t i)
{
  debug_push (strbuf ("[%zu]", i));
}

//-----------------------------------------------------------------------

void
json_encoder_t::set_top (ptr<pub3::expr_t> x)
{
  m_obj_stack.back () = x;
  m_ref_stack.back ()->set (x);
}

//-----------------------------------------------------------------------

void
json_encoder_t::push_ref (ptr<pub3::obj_ref_t> r)
{
  m_obj_stack.push_back (NULL);
  m_ref_stack.push_back (r);
}

//-----------------------------------------------------------------------

void
json_encoder_t::flush (xdrsuio *xs)
{
  // XXX error check and also pad!!!
  ptr<pub3::expr_t> x = m_root_ref->get ();
  str s = x->to_str (true);
  xdr_putpadbytes (m_x, s.cstr (), s.len ());
  xs->hold_onto (s);
}

//-----------------------------------------------------------------------

void
json_encoder_t::pop_ref ()
{
  m_obj_stack.pop_back ();
  m_ref_stack.pop_back ();
}

//-----------------------------------------------------------------------

bool
json_encoder_t::enter_pointer (bool &nonnil)
{
  bool ret = true;
  ptr<pub3::expr_list_t> l = pub3::expr_list_t::alloc ();
  set_top (l);
  if (nonnil) {
    enter_slot (0);
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
json_encoder_t::exit_pointer (bool nonnil)
{
  if (nonnil) {
    exit_slot (0);
  }
  return true;
}

//-----------------------------------------------------------------------

json_XDR_t::~json_XDR_t ()
{
  if (m_err_msg) {
    warn << "json-rpc decoding error: " << m_err_msg << "\n";
  }
}

//-----------------------------------------------------------------------

void json_fetch_constants_t::collect (const char *k, xdr_procpair_t p)
{ m_procpairs.insert (k, p); }

//-----------------------------------------------------------------------

void 
json_fetch_constants_t::collect (const char *key, int i, rpc_constant_type_t t)
{
  rpc_int_constant_t c;
  c.name = key;
  c.value = i;

  rpc_int_constants_t *v = NULL;

  switch (t) {
  case RPC_CONSTANT_PROG: v = &m_set.progs; break;
  case RPC_CONSTANT_VERS: v = &m_set.vers; break;
  case RPC_CONSTANT_PROC: v = &m_set.procs; break;
  case RPC_CONSTANT_ENUM: v = &m_set.enums; break;
  case RPC_CONSTANT_POUND_DEF: v = &m_set.pound_defs; break;
  default: break;
  }

  if (v) { v->push_back (c); }
}

//-----------------------------------------------------------------------

json_fetch_constants_t::json_fetch_constants_t ()
{
  global_rpc_constant_collect (this);
}

//=======================================================================

const rpc_program json_introspection_server_t::s_prog 
   = json_introspection_prog_1;

//-----------------------------------------------------------------------

bool
json_introspection_server_t::is_associated (ptr<axprt> x)
{
  ptr<xhinfo> xi = xhinfo::lookup (x);
  bool ret = false;
  if (xi) {
    ret = xi->stab[progvers (s_prog.progno, s_prog.versno)];
  }
  return ret;
}

//-----------------------------------------------------------------------

void
json_XDR_dispatch_t::v_asrv_alloc (ptr<axprt> x)
{
  if (!json_introspection_server_t::is_associated (x)) {
    vNew json_introspection_server_t (x);
  }
}

//-----------------------------------------------------------------------

json_introspection_server_t::json_introspection_server_t (ptr<axprt> x)
  : m_x (x), 
    m_srv (asrv::alloc (x, s_prog, 
			wrap (this, &json_introspection_server_t::dispatch),
			false))
{}

//-----------------------------------------------------------------------

void
json_introspection_server_t::dispatch (svccb *sbp)
{
  if (!sbp) { 
    delete this;
  } else {
    switch (sbp->proc ()) {
    case JSON_INTROSPECTION_FETCH_CONSTANTS:
      {
	rpc::json_introspection_prog_1::
	  json_introspection_fetch_constants_srv_t<svccb> srv (sbp);
	srv.reply (constant_set ());
      }
      break;
    default:
      sbp->reject (PROC_UNAVAIL);
      break;
    }
  }
}

//-----------------------------------------------------------------------

ptr<json_fetch_constants_t>
json_fetch_constants_t::get_singleton_obj ()
{
  static ptr<json_fetch_constants_t> ret;
  if (!ret) {
    ret = New refcounted<json_fetch_constants_t> ();
  }
  return ret;
}

//-----------------------------------------------------------------------

const rpc_constant_set_t &
json_introspection_server_t::constant_set ()
{
  return json_fetch_constants_t::get_singleton_obj ()->constant_set ();
}

//-----------------------------------------------------------------------

bool
json_fetch_constants_t::lookup_procpair (str s, xdr_procpair_t *out)
{
  xdr_procpair_t *xp;
  if ((xp = m_procpairs[s])) { *out = *xp; }
  return xp;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t> 
xdropq2json (str typ, const str &xdr_opq)
{
  ptr<pub3::expr_t> ret;
  xdr_procpair_t pp (NULL, NULL);
  bool found = 
    json_fetch_constants_t::get_singleton_obj()->lookup_procpair (typ, &pp);

  if (found) {
#define BUFSZ 4
    // We need this dummy just as a result of the extensible_rpc v_XDR_t
    // base class.  It really shouldn't need to do anything...

    xdrmem m (xdr_opq.cstr(), xdr_opq.len ());
    XDR *xm = &m;
    void *obj = (*pp.alloc)();
    bool ok = (*pp.proc) (xm, obj);
    if (ok) {

      xdrsuio xs (XDR_ENCODE, false);
      XDR *xsp = &xs;
      
      ptr<json_encoder_t> e = 
	json_XDR_dispatch_t::get_singleton_obj ()->alloc_encoder (xsp);
      ptr<v_XDR_t> vx = e;

      ok = (*pp.proc) (xsp, obj);
      if (ok) {
	ret = e->root_obj ();
	assert (ret);
      }
    }
    xdr_delete (pp.proc, obj);
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
json2xdropq (str t, str *out, ptr<const pub3::expr_t> cin)
{
  xdr_procpair_t pp (NULL, NULL);
  bool found = 
    json_fetch_constants_t::get_singleton_obj()->lookup_procpair (t, &pp);
  bool ret = false;
  if (found) {
#define BUFSZ 4
    char buf[BUFSZ];
    xdrmem x (buf, BUFSZ, XDR_DECODE);
#undef BUFSZ
    
    XDR *xd = &x;
    
    ptr<json_decoder_t> jd = 
      json_XDR_dispatch_t::get_singleton_obj ()->alloc_decoder (xd);
    
    jd->init_decode (cin->cast_hack_copy ());
    
    // run the standard str2xdr stuff
    void *obj = (*pp.alloc)();
    ret = (*pp.proc) (xd, obj);
    if (ret) {
      xdrsuio xs (XDR_ENCODE, false);
      XDR *xe = &xs;
      ret = (*pp.proc) (xe, obj);
      if (ret) {
	mstr m (xs.uio ()->resid ());
	xs.uio ()->copyout (m);
	*out = m;
      }
    }
    xdr_delete (pp.proc, obj);
  }
  return ret;
}


//-----------------------------------------------------------------------
