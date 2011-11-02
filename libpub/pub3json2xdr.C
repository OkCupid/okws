

#include "pub.h"
#include "pub3.h"

//-----------------------------------------------------------------------

static void
str2json (str s, xpub3_json_str_t *x)
{
  if (s) {
    x->setsize (s.len ());
    memcpy (x->base (), s.cstr (), s.len ());
  }
}

//-----------------------------------------------------------------------

str
json2str (const xpub3_json_str_t &k)
{
  mstr m (k.size ());
  memcpy (m.cstr(), k.base (), k.size ());
  m.setlen (k.size ());
  return m;
}

//-----------------------------------------------------------------------

bool
pub3::expr_t::to_xdr (xpub3_json_t *j) const
{
  j->set_typ (XPUB3_JSON_ERROR);
  return false;
}

//-----------------------------------------------------------------------

bool
pub3::expr_cow_t::to_xdr (xpub3_json_t *j) const
{
  bool ret;
  ptr<const expr_t> x = const_ptr ();
  if (x) {
    ret = x->to_xdr (j);
  } else {
    j->set_typ (XPUB3_JSON_ERROR);
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::to_xdr (xpub3_json_t *j) const
{
  j->set_typ (XPUB3_JSON_STRING);
  str2json (_val, j->json_string);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_bool_t::to_xdr (xpub3_json_t *j) const
{
  j->set_typ (XPUB3_JSON_BOOL);
  *j->json_bool = _b;
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_int_t::to_xdr (xpub3_json_t *j) const
{
  if (_val >= int64_t (INT32_MIN) && _val <= int64_t (INT32_MAX)) {
    j->set_typ (XPUB3_JSON_INT32);
    *j->json_int32 = _val;
  } else if ( _val >= 0 && _val <= int64_t (UINT32_MAX)) {
    j->set_typ (XPUB3_JSON_UINT32);
    *j->json_uint32 = _val;
  } else {
    j->set_typ (XPUB3_JSON_INT64);
    *j->json_int64 = _val;
  }
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_uint_t::to_xdr (xpub3_json_t *j) const
{
  if (_val <= u_int64_t (UINT32_MAX)) {
    j->set_typ (XPUB3_JSON_UINT32);
    *j->json_uint32 = _val;
  } else {
    j->set_typ (XPUB3_JSON_UINT64);
    *j->json_uint64 = _val;
  }
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_double_t::to_xdr (xpub3_json_t *j) const
{
  j->set_typ (XPUB3_JSON_DOUBLE);
  return to_xdr (j->json_double);
}

//-----------------------------------------------------------------------

bool
pub3::expr_list_t::to_xdr (xpub3_json_t *j) const
{
  j->set_typ (XPUB3_JSON_LIST);
  j->json_list->entries.setsize (size ());
  for (size_t i = 0; i < size (); i++) {
    ptr<const expr_t> x = (*this)[i];
    if (x) { x->to_xdr (&j->json_list->entries[i]); }
    else   { j->json_list->entries[i].set_typ (XPUB3_JSON_NULL); }
  }
  return true;
}

//-----------------------------------------------------------------------

static int
xjp_cmp (const void *cva, const void *cvb)
{
  void *va = const_cast<void *> (cva);
  void *vb = const_cast<void *> (cvb);
  const xpub3_json_pair_t *a = static_cast<xpub3_json_pair_t *> (va);
  const xpub3_json_pair_t *b = static_cast<xpub3_json_pair_t *> (vb);

  size_t len = min<size_t> (a->key.size (), b->key.size ());
  int r = memcmp (a->key.base (), b->key.base (), len);

  // We really shouldn't have equality!
  if (r == 0) { r = a->key.size () - b->key.size (); }

  return r;
}

//-----------------------------------------------------------------------

static void
sort (xpub3_json_pairs_t &l)
{
  qsort (l.base (), l.size (), sizeof (xpub3_json_pair_t), xjp_cmp);
}

//-----------------------------------------------------------------------

bool
pub3::expr_dict_t::to_xdr (xpub3_json_t *j) const
{
  j->set_typ (XPUB3_JSON_DICT);
  const_iterator_t it (*this);
  const str *key;
  ptr<expr_t> x;

  while ((key = it.next (&x))) {
    xpub3_json_pair_t jp;
    str2json (*key, &jp.key);
    if (x) {
      jp.value.alloc ();
      x->to_xdr (jp.value);
    }
    j->json_dict->entries.push_back (jp);
  }
  sort (j->json_dict->entries);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_null_t::to_xdr (xpub3_json_t *j) const
{
  j->set_typ (XPUB3_JSON_NULL);
  return true;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t> pub3::expr_t::alloc (const xpub3_json_t &x)
{ return alloc (&x); }

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
pub3::expr_t::alloc (const xpub3_json_t *x)
{
  ptr<expr_t> ret;
  if (x) {
    switch (x->typ) {
    case XPUB3_JSON_BOOL:
      ret = expr_bool_t::alloc (*x->json_bool);
      break;
    case XPUB3_JSON_NULL:
      ret = expr_null_t::alloc ();
      break;
    case XPUB3_JSON_INT32:
      ret = expr_int_t::alloc (*x->json_int32);
      break;
    case XPUB3_JSON_UINT32:
      ret = expr_int_t::alloc (*x->json_uint32);
      break;
    case XPUB3_JSON_INT64:
      ret = expr_int_t::alloc (*x->json_int64);
      break;
    case XPUB3_JSON_UINT64:
      ret = expr_uint_t::alloc (*x->json_uint64);
      break;
    case XPUB3_JSON_LIST:
      ret = expr_list_t::alloc (*x->json_list);
      break;
    case XPUB3_JSON_DICT:
      ret = expr_dict_t::alloc (*x->json_dict);
      break;
    case XPUB3_JSON_STRING:
      ret = expr_str_t::alloc (*x->json_string);
      break;
    case XPUB3_JSON_DOUBLE:
      ret = expr_double_t::alloc (*x->json_double);
      break;
    default:
      break;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_str_t>
pub3::expr_str_t::alloc (const xpub3_json_str_t &x)
{
  str s = json2str (x);
  return alloc (s);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_list_t>
pub3::expr_list_t::alloc (const xpub3_json_list_t &x)
{
  ptr<pub3::expr_list_t> r = New refcounted<expr_list_t> ();
  for (size_t i = 0; i < x.entries.size (); i++) {
    ptr<expr_t> e = expr_t::alloc (x.entries[i]);
    r->push_back (e);
  }
  return r;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_dict_t>
pub3::expr_dict_t::alloc (const xpub3_json_dict_t &x)
{
  ptr<pub3::expr_dict_t> d = pub3::expr_dict_t::alloc();
  for (size_t i = 0; i < x.entries.size (); i++) {
    const xpub3_json_pair_t &jp = x.entries[i];
    ptr<expr_t> v = expr_t::alloc (jp.value);
    str s = json2str (jp.key);
    if (s) { d->insert (s, v); }
  }
  return d;
}

//-----------------------------------------------------------------------
