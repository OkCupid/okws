
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

template<> void
big_endian(signed char& out, u_int8_t* in, size_t n) {
    out = 0;
    out |= (signed char) in[0];
}

template<> void
big_endian(unsigned char& out, u_int8_t* in, size_t n) {
    out = 0;
    out |= (unsigned char) in[0];
}

//-----------------------------------------------------------------------

template<class T, size_t N = sizeof (T) > 
class floater_t {
  union {
    u_int8_t b[sizeof (N)];
    T v;
  } u;

public:

  floater_t () { u.v = 0; }
  floater_t (T v) { u.v = v; }
  u_int8_t *buf () { return u.b; }
  size_t size () const { return N; }

  void swap () {
    for (size_t i = 0; i < N/2; i++) {
      size_t j = N - 1 - i;
      u_int8_t tmp = u.b[j];
      u.b[j] = u.b[i];
      u.b[i] = tmp;
    }
  }

  T val () const { return u.v; }
};

//=======================================================================

class msgpack_t {
public:
  msgpack_t (str m) : 
    _buf (m), _cp (m.cstr ()), _ep (_cp + m.len ()), _my_errno (0) {}

  ptr<pub3::expr_t> unpack ();
  size_t len () const { return _cp - _buf; }
  int my_errno () const { return _my_errno; }

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
      ret = true;
    } else {
      _my_errno = EAGAIN;
    }
    return ret;
  }

  void put_back (size_t n = 1) { _cp -= n; }

  str _buf;
  const char *_cp;
  const char *_ep;
  int _my_errno;
};

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
msgpack_t::unpack_negative_fixnum ()
{
  int8_t b;
  bool ok = unpack_int (&b);
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
  u_int8_t n = b ^ (0xa0);
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

    for (u_int8_t i = 0; i < 0xff; i++) { tab[i] = NULL; }

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

    for (u_int16_t i = 0xe0; i <= 0xff; i++) P(i, negative_fixnum);

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
    unpack_hook_t h = unpack_tab()[b];
    if (h) {
      ret = (this->*h) ();
    } else {
      _my_errno = EINVAL;
    }
  }
  return ret;
}

//=======================================================================


namespace pub3 { 
  namespace msgpack {

    ptr<expr_t>
    decode (str msg, int *errno_p, size_t *len_p) 
    {
      msgpack_t b (msg);
      ptr<expr_t> ret = b.unpack ();
      if (errno_p) { *errno_p = b.my_errno (); }
      if (len_p) { *len_p = b.len (); }
      return ret;
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
  }
};

//=======================================================================

pub3::msgpack::outbuf_t::outbuf_t ()
  : _tlen (0x100),
    _tmp (New mstr (_tlen)), 
    _tp (_tmp->cstr ()), 
    _ep (_tp + _tlen) {}

//-----------------------------------------------------------------------

pub3::msgpack::outbuf_t::~outbuf_t () { delete _tmp; }

//-----------------------------------------------------------------------

void
pub3::msgpack::outbuf_t::flush (bool r) 
{
  if (_tmp && _tp > _tmp->cstr ()) {
    _tmp->setlen (_tp - _tmp->cstr ());
    str s = *_tmp;
    _b << s;
    _b.hold_onto (s);
    delete _tmp;
    _tmp = NULL;
    if (r) { ready (); }
  }
}

//-----------------------------------------------------------------------

void
pub3::msgpack::outbuf_t::ready ()
{
  if (!_tmp) {
    _tmp = New mstr (_tlen);
    _tp = _tmp->cstr ();
    _ep = _tp + _tlen;
  }
}

//-----------------------------------------------------------------------

void
pub3::msgpack::outbuf_t::put_str (str s)
{
  flush ();
  _b << s;
  _b.hold_onto (s);
}

//-----------------------------------------------------------------------

void
pub3::msgpack::outbuf_t::put_bytes (u_int8_t *b, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    put_byte (b[i]);
  }
}

//-----------------------------------------------------------------------

void
pub3::msgpack::outbuf_t::put_byte (u_int8_t b)
{
  ready ();
  assert (_tp < _ep);
  *(_tp++) = b;
  if (_tp == _ep) { flush (); }
}

//-----------------------------------------------------------------------

str
pub3::msgpack::outbuf_t::to_str ()
{
  flush (false);
  return _b;
}

//-----------------------------------------------------------------------

void
pub3::msgpack::outbuf_t::encode_negative_int (int64_t i)
{
  assert (i < 0);

  if (i >= -32) {
    put_byte (i);
  } else if (i >= -128){ 
    put_byte (0xd0);
    put_byte (i);
  } else if (i >= -32768) { 
    put_byte (0xd1);
    int16_t s = i;
    put_int (s);
  } else if (i >= -2147483648) {
    put_byte (0xd2);
    int32_t w = i;
    put_int (w);
  } else {
    put_byte (0xd3);
    int64_t q = i;
    put_int (q);
  }
}

//-----------------------------------------------------------------------

void
pub3::msgpack::outbuf_t::encode_positive_int (u_int64_t i)
{

  if (i <= 0x7f) { 
    put_byte (i); 
  } else if (i <= 0xff) { 
    put_byte (0xcc);
    put_byte (i);
  } else if (i <= 0xffff) { 
    put_byte (0xcd);
    u_int16_t s = i;
    put_int (s);
  } else if (i <= 0xffffffff) {
    put_byte (0xce);
    u_int32_t w = i;
    put_int (w);
  } else {
    put_byte (0xcf);
    put_int (i);
  }
}

//-----------------------------------------------------------------------

void 
pub3::msgpack::outbuf_t::encode_raw_len (size_t len, u_int8_t b, 
                     u_int8_t s, u_int8_t l)
{
  if (len <= 0x1f) {
    b |= len;
    put_byte (b);
  } else if (len <= 0xffff) {
    put_byte (s);
    u_int16_t i = len;
    put_int (i);
  } else {
    put_byte (l);
    u_int32_t i = len;
    put_int (i);
  }
}

//-----------------------------------------------------------------------

void 
pub3::msgpack::outbuf_t::encode_len (size_t len, u_int8_t b, u_int8_t s, 
				     u_int8_t l)
{
  if (len <= 0xf) {
    b |= len;
    put_byte (b);
  } else if (len <= 0xffff) {
    put_byte (s);
    u_int16_t i = len;
    put_int (i);
  } else {
    put_byte (l);
    u_int32_t i = len;
    put_int (i);
  }
}
//-----------------------------------------------------------------------

void
pub3::msgpack::outbuf_t::encode_str (str s)
{
  assert (s);
  size_t l = s.len ();
  encode_raw_len (l, 0xa0, 0xda, 0xdb);
  put_str (s);
}

//=======================================================================

bool
pub3::expr_cow_t::to_msgpack (pub3::msgpack::outbuf_t *j) const
{
  bool ret;
  ptr<const expr_t> x = const_ptr ();
  if (x) {
    ret = x->to_msgpack (j);
  } else {
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_null_t::to_msgpack (pub3::msgpack::outbuf_t *j) const
{
  j->put_byte (0xc0);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_bool_t::to_msgpack (pub3::msgpack::outbuf_t *j) const
{
  j->put_byte (_b ? 0xc3 : 0xc2);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::to_msgpack (pub3::msgpack::outbuf_t *j) const
{
  bool ret = true;
  if (!_val) {
    ret = false;
  } else {
    j->encode_str (_val);
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_int_t::to_msgpack (pub3::msgpack::outbuf_t *j) const
{
  if (_val < 0) {
    j->encode_negative_int (_val);
  } else {
    j->encode_positive_int (_val);
  }
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_uint_t::to_msgpack (pub3::msgpack::outbuf_t *j) const
{
  j->encode_positive_int (_val);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_double_t::to_msgpack (pub3::msgpack::outbuf_t *j) const
{
  floater_t<double> f (_val);;
  f.swap ();
  j->put_byte(0xcb);
  j->put_bytes (f.buf (), f.size ());
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_list_t::to_msgpack (pub3::msgpack::outbuf_t *j) const
{
  size_t l = size ();
  j->encode_len (l, 0x90, 0xdc, 0xdd);

  for (size_t i = 0; i < l; i++) {
    (*this)[i]->to_msgpack (j);
  }

  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_dict_t::to_msgpack (pub3::msgpack::outbuf_t *j) const
{ 
  size_t l = size ();
  j->encode_len (l, 0x80, 0xde, 0xdf);

  ptr<const_iterator_t> it = iter ();
  ptr<expr_t> x;
  const str *keyp;
  
  while ((keyp = it->next (&x))) {
    scalar_obj_t so (*keyp);
    u_int64_t ku;
    int64_t ki;
    if (so.to_uint64 (&ku)) {
      j->encode_positive_int (ku);
    } else if (so.to_int64 (&ki)) {
      assert (ki < 0);
      j->encode_negative_int (ki);
    } else {
      j->encode_str (*keyp);
    }
    x->to_msgpack (j);
  }
  return true;
}

//-----------------------------------------------------------------------

