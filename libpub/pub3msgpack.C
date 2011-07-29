
#include "pub3msgpack.h"
#include "pub3.h"
#include "qhash.h"

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

//=======================================================================

class msgpack_t {
public:
  msgpack_t (str m) : _buf (m), _cp (m.cstr ()), _ep (_cp + m.len ()) {}

  ptr<pub3::expr_t> unpack ();

  typedef ptr<pub3::expr_t> (msgpack_t::*unpack_hook_t) (void);
  typedef msgpack_t::unpack_hook_t unpack_tab_t[256];

private:

  static unpack_tab_t &unpack_tab ();

  ptr<pub3::expr_t> unpack_positive_fixnum ();
  ptr<pub3::expr_t> unpack_negative_fixnum ();
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

  ptr<pub3::expr_t> unpack_array (size_t n);
  ptr<pub3::expr_t> unpack_map (size_t n);
  ptr<pub3::expr_t> unpack_raw (size_t n);

  template<class T> bool 
  unpack_int (T *v)
  {
    u_int8_t buf[sizeof (T)];
    bool ok = get_bytes (buf, sizeof (T));
    if (ok) {
      big_endian (*v, buf, sizeof (T));
    } 
    return ok;
  }

  void consume_byte ();
  bool get_byte (u_int8_t *b);
  bool peek_byte (u_int8_t *b);


  template<class T>
  bool get_bytes (T *buf, size_t n)
  {
    bool ret = false;
    if (_cp + n <= _ep) {
      memcpy (buf, _cp, n);
      _cp += n;
    }
    return ret;
  }

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

public:

  u_int8_t *buf () { return u.b; }
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
msgpack_t::unpack_negative_fixnum ()
{
  u_int8_t b;
  bool ok = get_byte (&b);
  assert (ok);
  return pub3::expr_int_t::alloc (b);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_positive_fixnum ()
{
  u_int8_t b;
  bool ok = get_byte (&b);
  assert (ok);
  return pub3::expr_int_t::alloc (b);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_double ()
{
  consume_byte();
  ptr<pub3::expr_double_t> ret;
  floater_t<double> f;
  bool ok = get_bytes (f.buf (), f.size ());
  if (ok) {
    // Endian-swap ?
    f.swap ();
    ret = pub3::expr_double_t::alloc (f.val());
  }

  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_float ()
{
  consume_byte();
  ptr<pub3::expr_double_t> ret;
  floater_t<float> f;
  bool ok = get_bytes (f.buf (), f.size ());
  if (ok) {
    // Endian-swap ?
    f.swap ();
    ret = pub3::expr_double_t::alloc (f.val());
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_map (size_t n)
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
      else { d->insert (ks, v); }
    }
  }
  if (!ok) { 
    warn << "msgpack::unpack failed in unpack_map\n";
    d = NULL;
  }
  return d;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_map32 ()
{
  consume_byte ();
  u_int32_t n;
  bool ok = unpack_int (&n);
  ptr<pub3::expr_t> ret;
  if (ok) {
    ret = unpack_map (n);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_map16 ()
{
  consume_byte ();
  u_int16_t n;
  bool ok = unpack_int (&n);
  ptr<pub3::expr_t> ret;
  if (ok) {
    ret = unpack_map (n);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_fix_map ()
{
  u_int8_t b;
  bool ok = get_byte (&b);
  assert (ok);

  // Strip off the top bit
  u_int8_t n = b ^ (0x80);
  return unpack_map (n);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_raw (size_t n)
{
  mstr m (n+1);
  m[n] = 0;
  bool ok;
  ptr<pub3::expr_t> ret;
  ok = get_bytes (m.cstr (), n);
  if (ok) {
    m.setlen (n);
    ret = pub3::expr_str_t::alloc (m);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_raw32 ()
{
  consume_byte ();
  u_int32_t s;
  bool ok = unpack_int (&s);
  ptr<pub3::expr_t> ret;
  if (ok) {
    ret = unpack_raw (s);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_raw16 ()
{
  consume_byte ();
  u_int16_t s;
  bool ok = unpack_int (&s);
  ptr<pub3::expr_t> ret;
  if (ok) {
    ret = unpack_raw (s);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_fix_raw ()
{
  u_int8_t b;
  bool ok = get_byte (&b);
  assert (ok);

  // Strip off the top bit
  u_int8_t n = b ^ (0xa);
  return unpack_raw (n);
}

//-----------------------------------------------------------------------

#define UPI(nm, ctyp, oktyp)				\
  ptr<pub3::expr_t>					\
  msgpack_t::unpack_##nm ()				\
  {							\
    ptr<pub3::expr_t> ret;				\
    consume_byte ();					\
    ctyp b;						\
    bool ok = unpack_int (&b);				\
    if (ok) {						\
      ret = pub3::expr_##oktyp##_t::alloc (b);		\
    }							\
    return ret;						\
  }

UPI(int8, int8_t, int)
UPI(uint8, u_int8_t, int)
UPI(int16, int16_t, int)
UPI(uint16, u_int16_t, int)
UPI(int32, int32_t, int)
UPI(uint32, u_int32_t, int)
UPI(int64, int64_t, int)
UPI(uint64, u_int64_t, uint)

#undef UPI

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_false ()
{
  consume_byte ();
  return pub3::expr_bool_t::alloc (false);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_true ()
{
  consume_byte ();
  return pub3::expr_bool_t::alloc (true);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_nil ()
{
  consume_byte ();
  return pub3::expr_null_t::alloc ();
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_array (size_t n)
{
  ptr<pub3::expr_list_t> ret;
  bool ok = true;
  ret = pub3::expr_list_t::alloc ();
  ret->reserve (n);
  for (size_t i = 0; ok && i < n; i++) {
    ptr<pub3::expr_t> x = unpack ();
    if (!x) { 
      ok = false;
    } else {
      ret->push_back (x);
    }
  }
  if (!ok) { ret = NULL; }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t> 
msgpack_t::unpack_array32 ()
{
  consume_byte ();
  u_int32_t n;
  bool ok = unpack_int (&n);
  ptr<pub3::expr_t> ret;
  if (ok) {
    ret = unpack_array (n);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t> 
msgpack_t::unpack_array16 ()
{
  consume_byte ();
  u_int16_t n;
  bool ok = unpack_int (&n);
  ptr<pub3::expr_t> ret;
  if (ok) {
    ret = unpack_array (n);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_fix_array ()
{
  u_int8_t b;
  bool ok = get_byte (&b);
  assert (ok);
  b ^= 0x90;
  return unpack_array (b);
}

//-----------------------------------------------------------------------

msgpack_t::unpack_tab_t &
msgpack_t::unpack_tab () 
{
  static unpack_tab_t tab;
  static bool init;
  if (!init) {

#define P(code, obj) \
    tab[code] = &msgpack_t::unpack_##obj;

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

    for (u_int8_t i = 0xe0; i <= 0xff; i++) P(i, negative_fixnum);

#undef P
    init = true;
  }

  return tab;
}

//-----------------------------------------------------------------------

void
msgpack_t::consume_byte ()
{
  u_int8_t dummy;
  bool ok = get_byte (&dummy);
  assert (ok);
}

//-----------------------------------------------------------------------

bool
msgpack_t::get_byte (u_int8_t *b)
{
  bool ret = peek_byte (b);
  if (ret) _cp++;
  return ret;
}

//-----------------------------------------------------------------------

bool 
msgpack_t::peek_byte (u_int8_t *b)
{
  bool ret = false;
  if (_cp < _ep) { *b = *_cp; ret = true; }
  return ret;
}

//=======================================================================

ptr<pub3::expr_t>
msgpack_t::unpack ()
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


namespace pub3 { 
  namespace msgpack {
    
    ptr<expr_t>
    decode (str msg) 
    {
      msgpack_t b (msg);
      return b.unpack ();
    };
    
    str
    encode (ptr<const expr_t> x)
    {
      outbuf_t b;
      str ret;
      if (x->to_msgpack (&b)) {
	ret = b.to_str ();
      }
      return ret;
    }
    
  };
};

//=======================================================================

