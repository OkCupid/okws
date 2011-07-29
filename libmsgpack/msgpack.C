#include "okmsgpack.h"
#include "qhash.h"

namespace msgpack {

#if 0
}
#endif

//=======================================================================

class buffer_t {
public:
  buffer_t (str m) : _buf (m), _cp (m.cstr ()), _ep (_cp + m.len ()) {}

  ptr<pub3::expr_t> unpack ();

  typedef (ptr<pub3::expr_t>) (buffer_t::*unpack_hook) (void);
  typedef buffer_t::unpack_hook_t unpack_tab_t[256];

private:

  static unpack_tab_t &unpack_tab ();

  ptr<pub3::expr_t> unpack_positive_fixnum ();
  ptr<pub3::expr_t> unpack_fix_map ();
  ptr<pub3::expr_t> unpack_fix_array ();
  ptr<pub3::expr_t> unpack_fix_raw ();
  ptr<pub3::expr_t> unpack_float ();
  ptr<pub3::expr_t> unpack_double ();
  ptr<pub3::expr_t> unpack_uint8 ();
  ptr<pub3::expr_t> unpack_uint16 ();
  ptr<pub3::expr_t> unpack_uint32 ();
  ptr<pub3::expr_t> unpack_uint64 ();
  ptr<pub3::expr_t> unpack_int8 ();
  ptr<pub3::expr_t> unpack_int16 ();
  ptr<pub3::expr_t> unpack_int32 ();
  ptr<pub3::expr_t> unpack_int64 ();
  ptr<pub3::expr_t> unpack_raw16 ();
  ptr<pub3::expr_t> unpack_raw32 ();
  ptr<pub3::expr_t> unpack_array16 ();
  ptr<pub3::expr_t> unpack_array32 ();
  ptr<pub3::expr_t> unpack_map16 ();
  ptr<pub3::expr_t> unpack_map32 ();

  ptr<pub3::expr_t> unpack_nil ();
  ptr<pub3::expr_t> unpack_true ();
  ptr<pub3::expr_t> unpack_false ();

  ptr<pub3::expr_t> unpack_dict (size_t n);

  void consume_byte ();
  bool get_byte (u_int8_t *b);
  bool peek_byte (u_int8_t *b);
  bool get_bytes (u_int8_t *buf, size_t n);
  void put_back (size_t n = 1) { _cp -= n; }

  str _buf;
  const char *_cp;
  const char *_ep;
};

//-----------------------------------------------------------------------

template<class T, size_t N = sizeof (T) > 
class floater_t {
  union {
    u_int8_t b[sizeof (N)];
    T v;
  } u;

  u_int_t *buf { return u.b; }
  size_t size () const { return N; }

  void swap () {
    for (size_t i = 0; i < N/2; i++) {
      size_t j = N - 1 - j;
      u_int8_t tmp = u.b[j];
      u.b[j] = u.b[i];
      u.b[i] = tmp;
    }
  }

  T val () const { return u.v; }
};

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_positive_fixnum ()
{
  u_int8_t b;
  bool ok = get_byte (&b);
  assert (ok);
  return pub3::expr_int_t::alloc (b);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_double ()
{
  consume_byte();
  ptr<pub3::expr_double_t> ret;
  floater_t f<double>;
  bool ok = get_bytes (f.buf (), f.size ());
  if (ok) {
    // Endian-swap ?
    buf.swap ();
    ret = pub3::expr_double_t::alloc (buf.val());
  }

  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_float ()
{
  consume_byte();
  ptr<pub3::expr_double_t> ret;
  floater_t f<float>;
  bool ok = get_bytes (f.buf (), f.size ());
  if (ok) {
    // Endian-swap ?
    buf.swap ();
    ret = pub3::expr_double_t::alloc (buf.val());
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_dict (size_t n)
{
  ptr<pub3::expr_dict_t> d = pub3::expr_dict_t::alloc ();
  bool ok = true;
  for (size_t i = 0; ok && i < n; i++) {
    ptr<pub3::expr_t> k = unpack ();
    str ks;
    if (!k) { ok = false; }
    else { ks = k->to_str (); }
    if (ks) {
      ptr<pub3::expr_t> v = unpack ();
      if (!v) { ok = false; }
      else { v->insert (ks, v); }
    }
  }
  if (!ok) { 
    warn << "msgpack::unpack failed in unpack_dict\n";
    d = NULL;
  }
  return d;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_fix_map ()
{
  u_int8_t b;
  bool ok = get_byte (&b);
  assert (ok);

  // Strip off the top bit
  u_int8_t n = b ^ (0x80);
  return unpack_dict (n);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_raw (size_t n)
{
  mstr m[n+1];
  m[n] = 0;
  bool ok;
  ptr<pub3::expr_t> ret;
  ok = get_bytes (m.cstr (), n);
  if (ok) {
    m.setlen (n);
    ret = expr_str_t::alloc (m);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_raw16 ()
{
  consume_bytes ();
  u_int16_t s;
  bool ok = get_uint16 (&s);
  ptr<pub3::expr_t> ret;
  if (ok) {
    ret = unpack_raw (s);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_fix_raw ()
{
  u_int8_t b;
  bool ok = get_byte (&b);
  assert (ok);

  // Strip off the top bit
  u_int8_t n = b ^ (0xa);
  return unpack_raw (n);
}

//-----------------------------------------------------------------------

template<class T> void
big_endian (T &out, u_int8_t *in, size_t n)
{
  out = 0;
  for (size_t i = 0; i < n; i++) {
    if (i != 0) { out <<= 8; }
    out |= (T )in[i];
  }
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_uint64 ()
{
  ptr<pub3::expr_t> ret;
  consume_byte ();
  u_int8_t b[8];
  bool ok = unpack_bytes (&b, 8);
  if (ok) {
    u_int64_t v;
    big_endian (v, b, 8);
    ret = pub3::expr_uint_t::alloc (v);
  }
  return ret;
}

//-----------------------------------------------------------------------

bool 
buffer_t::unpack_uint16 (u_int16_t *v)
{
  u_int8_t b[2];
  bool ok = unpack_bytes (&b, 2);
  if (ok) {
    big_endian (&v, b, 2);
  }
  return ok;
}


//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_int16 ()
{
  ptr<pub3::expr_t> ret;
  consume_byte ();
  u_int8_t b[2];
  bool ok = unpack_bytes (&b, 2);
  if (ok) {
    int16_t v;
    big_endian (v, b, 2);
    ret = pub3::expr_int_t::alloc (v);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_uint16 ()
{
  ptr<pub3::expr_t> ret;
  consume_byte ();
  u_int16_t s;
  if (unpack_uint16 (&s)) {
    ret = pub3::expr_int_t::alloc (s);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_int8 ()
{
  ptr<pub3::expr_t> ret;
  consume_byte ();
  u_int8_t ub;
  bool ok = unpack_byte (&ub);
  if (ok) {
    int8_t b = ub;
    ret = pub3::expr_int_t::alloc (b);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_uint8 ()
{
  ptr<pub3::expr_t> ret;
  consume_byte ();
  u_int8_t b;
  bool ok = unpack_byte (&b);
  if (ok) {
    ret = pub3::expr_int_t::alloc (b);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_false ()
{
  consume_byte ();
  return pub3::expr_bool_t::alloc (false);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_true ()
{
  consume_byte ();
  return pub3::expr_bool_t::alloc (true);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
buffer_t::unpack_nil ()
{
  consume_byte ();
  return pub3::expr_null_t::alloc ();
}

//-----------------------------------------------------------------------

static buffer_t::unpack_tab_t &
buffer_t::unpack_tab () 
{
  static unpack_tab_t tab;
  static bool init;
  if (!init) {

#define P(code, obj) \
    tab[code] = &buffer_t::unpack_##obj;

    for (u_int8_t i = 0x00; i <= 0x7f; i++) P(i, positive_fixnum);
    for (u_int8_t i = 0x80; i <= 0x8f; i++) P(i, fix_map);
    for (u_int8_t i = 0x90; i <= 0x9f; i++) P(i, fix_array);
    for (u_int8_t i = 0xa0; i <= 0xbf; i++) P(i, fix_raw);
    
    P(0xc0, nil);
    P(0xc2, false);
    P(0xc3, true);
    P(0xca, float);
    P(0xcb, double);
    P(0xcc, uint8);
    P(0xcd, uint16);
    P(0xce, uint32);
    P(0xcf, uint64);
    P(0xd0, int8);
    P(0xd1, int16);
    P(0xd2, int32);
    P(0xd3, int64);
    P(0xda, raw16);
    P(0xdb, raw32);
    P(0xdc, array16); 
    P(0xdd, array32); 
    P(0xde, map16); 
    P(0xdf, map32); 

    for (u_int8_t i = 0xe0; i <= 0xff; i++) P(i, unpack_negative_fixnum);

#undef P
    init = true;
  }

  return tab;
}


//-----------------------------------------------------------------------

bool
buffer_t::get_bytes (const char **buf, size_t n)
{
  bool ret = false;
  if (_cp + n <= _ep) {
    ret = true;
    *buf = _cp;
    _cp += n;
  }
  return ret;
}

//-----------------------------------------------------------------------

void
buffer_t::consume_byte ()
{
  u_int8_t dummy;
  bool ok = get_byte (&dummy);
  assert (ok);
}

//-----------------------------------------------------------------------

bool
buffer_t::get_byte (u_int8_t *b)
{
  bool ret = peek_byte (b);
  if (ret) _cp++;
  return ret;
}

//-----------------------------------------------------------------------

bool 
buffer_t::peek_byte (u_int8_t *b)
{
  bool ret = false;
  if (_cp < _ep) { *b = *_cp; ret = true; }
  return ret;
}

//=======================================================================

ptr<pub3::expr_t>
buffer_t::unpack ()
{
  ptr<pub3::expr_t> ret;
  u_int8_t b;
  bool ok = peek_byte (&b);
  if (ok) { 
    ret = (this->*(unpack_tab()[b])) ();
  }
  return ret;
}

//=======================================================================

ptr<pub3::expr_t>
buffer_t::

//=======================================================================

ptr<pub3::expr_t>
unpack (str msg) 
{
  buffer_t b (msg);
  return unpack (&b);
};


//=======================================================================

};
