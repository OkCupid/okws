#include "mybind.h"
#if HAVE_MYSQL

//-----------------------------------------------------------------------

#ifdef HAVE_MYSQL_BIND
void
mybind_str_t::bind (MYSQL_BIND *bnd, bool param)
{
  bnd->buffer_type = ft;
  bnd->buffer = buf;
  bnd->buffer_length = size;
  bnd->length = param ? &bnd->buffer_length : &len;
  bnd->is_null = param ? 0 : &nullfl;
}
#endif

//-----------------------------------------------------------------------

static void
static_to_qry (const char *buf, u_int size, MYSQL *m, strbuf *b, char **s,
	       u_int *l, char q)
{
  u_int len2 = (size << 1) + 1;
  if (*l < len2 || !*s) {
    *l = next2pow (len2);
    if (*s)
      delete [] *s;
    *s = New char[*l];
  }
  u_int rlen = mysql_real_escape_string (m, *s + 1, buf, size);
  (*s)[0] = (*s)[rlen + 1] = q;
  b->buf (*s, rlen + 2);
}

//-----------------------------------------------------------------------

void
mybind_str_t::to_qry (MYSQL *m, strbuf *b, char **s, u_int *l)
{
  static_to_qry (buf, size, m, b, s, l, '\'');
}

//-----------------------------------------------------------------------

void
mybind_var_t::to_qry (MYSQL *m, strbuf *b, char **s, u_int *l)
{
  static const char *quote = "`";
  if (_val && !strchr (_val.cstr (), quote[0])) {
    *b << quote << _val << quote;
  }
}

//-----------------------------------------------------------------------

str mybind_str_t::to_str () const { return str (buf, size); }

//-----------------------------------------------------------------------

void
mybind_sex_t::to_qry (MYSQL *m, strbuf *b, char **s, u_int *l)
{
  buf[1] = sex_to_char (sx);
  b->buf (buf, 3);
}

//-----------------------------------------------------------------------

str 
mybind_sex_t::to_str () const 
{
  char b[2];
  b[1] = 0;
  b[0] = sex_to_char (sx);
  return str (b, 2);
}

//-----------------------------------------------------------------------

u_int
next2pow (u_int i)
{
  u_int log = 0;
  while (i) {
    i = i >> 1;
    log++;
  }
  return (max (1 << (log + 1), BIND_BUF_MIN)); 
}

//-----------------------------------------------------------------------

static eft_t 
to_mysql_typ (const okdate_t &d)
{
  if (!d.err) {
    if (d.dt_tm & ( OK_DATE | OK_TIME)) {
      return MYSQL_TYPE_DATETIME;
    } else if (d.dt_tm & OK_DATE) {
      return MYSQL_TYPE_DATE;
    } else if (d.dt_tm & OK_TIME) {
      return MYSQL_TYPE_TIME;
    }
  }
  return MYSQL_TYPE_DATETIME;
}

static eft_t
to_mysql_typ (const x_okdate_t &x) 
{
  if (x.date.on && x.time.on) {
    return MYSQL_TYPE_DATETIME;
  } else if (x.date.on) {
    return MYSQL_TYPE_DATE;
  } else {
    return MYSQL_TYPE_TIME;
  }
}

//-----------------------------------------------------------------------

mybind_date_t::mybind_date_t (okdatep_t d) 
  : mybind_t (to_mysql_typ (*d)), datep (d), pntr (NULL), parsed (false),
    palloced (false) {}

//-----------------------------------------------------------------------

mybind_date_t::mybind_date_t (const okdate_t &d) 
  : mybind_t (to_mysql_typ (d)), 
    datep (New refcounted<okdate_t> (d)), 
    pntr (NULL), parsed (false),
    palloced (false) {}

//-----------------------------------------------------------------------

mybind_date_t::mybind_date_t (const x_okdate_t &d)
  : mybind_t (to_mysql_typ (d)), datep (okdate_t::alloc (d)), pntr (NULL),
    parsed (false), palloced (false) {}

//-----------------------------------------------------------------------

void
mybind_date_t::to_qry (MYSQL *m, strbuf *b, char **s, u_int *l)
{
  datep->to_strbuf (b, true, global_gmt_offset.get ());
}

//-----------------------------------------------------------------------

str
mybind_date_t::to_str () const 
{
  strbuf b;
  datep->to_strbuf (&b, true, global_gmt_offset.get ());
  return b;
}

//-----------------------------------------------------------------------

#ifdef HAVE_MYSQL_BIND
void
mybind_date_t::bind (MYSQL_BIND *bnd, bool param)
{
  bnd->buffer_type = ft;
  bnd->buffer = (char *)&tm;
  bnd->buffer_length = sizeof (tm);
  bnd->length = param ? &bnd->buffer_length : &len;
  bnd->is_null = param ? 0 : &nullfl;
}

//-----------------------------------------------------------------------

void
mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_t *x)
{
  x->date.set_on (true);
  x->time.set_on (true);
  mysql_to_xdr (tm, x->date.date);
  mysql_to_xdr (tm, x->time.time);
}

//-----------------------------------------------------------------------

void
mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_time_t *x)
{
  x->sec = tm.second;
  x->min = tm.minute;
  x->hour = tm.hour;
}

//-----------------------------------------------------------------------

void
mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_date_t*x)
{
  x->mday = tm.day;
  x->mon = tm.month;
  x->year = tm.year;
}

#endif

//-----------------------------------------------------------------------

bool
mybind_date_t::read_str (const char *c, unsigned long, eft_t typ)
{
  parsed = true;
  datep->set (c, global_gmt_offset.get ());
  if (pntr)
    pntr->parsed_assign (datep);
  return (!datep->err);
}

//-----------------------------------------------------------------------

mybind_param_t::mybind_param_t (const amysql_scalar_t &x)
{
  p = mybind_xdr_union_t::alloc (x, false);
}

//-----------------------------------------------------------------------

ptr<mybind_t>
mybind_xdr_union_t::alloc (const amysql_scalar_t &x, bool lenient)
{
  ptr<mybind_t> p;
  switch (x.typ) {
  case AMYSQL_TYPE_STRING:
    p = New refcounted<mybind_str_t> (*x.amysql_string);
    break;
  case AMYSQL_TYPE_OPAQUE:
    {
      str tmp (x.amysql_opaque->base (), x.amysql_opaque->size ());
      p = New refcounted<mybind_str_t> (tmp);
    }
    break;
  case AMYSQL_TYPE_INT:
    p = New refcounted<mybind_int_t> (*x.amysql_int);
    break;
  case AMYSQL_TYPE_DOUBLE:
    {
      double d = 0.0;
      if (convertdouble (*x.amysql_double, &d) || lenient) {
	p = New refcounted<mybind_double_t> (d);
      }
    }
    break;
  case AMYSQL_TYPE_BOOL:
    p = New refcounted<mybind_bool_t> (*x.amysql_bool);
    break;
  case AMYSQL_TYPE_DATE:
    p = New refcounted<mybind_date_t> (*x.amysql_date);
    break;
  case AMYSQL_TYPE_UINT64:
    p = New refcounted<mybind_u64_t> (*x.amysql_uint64);
    break;
  default:
    break;
  }
  return p;
}

//-----------------------------------------------------------------------

str
mybind_xdr_union_t::to_str () const
{
  ptr<mybind_t> b = mybind_xdr_union_t::alloc (_x, true);
  str ret;
  if (b) ret = b->to_str ();
  return ret;
}

//-----------------------------------------------------------------------

bool
mybind_xdr_union_t::read_str (const char *c, unsigned long l, eft_t typ)
{
  bool ret = false;
  switch (typ) {

  case MYSQL_TYPE_TINY:
  case MYSQL_TYPE_SHORT:
  case MYSQL_TYPE_LONG:
  case MYSQL_TYPE_INT24:
    {
      int tmp;
      if ((ret = convertint (c, &tmp))) {
	_pntr->set_typ (AMYSQL_TYPE_INT);
	*_pntr->amysql_int = tmp;
      }
    }
    break;

  case MYSQL_TYPE_LONGLONG:
    {
      u_int64_t tmp;
      if ((ret = convertuint (c, &tmp))) {
	_pntr->set_typ (AMYSQL_TYPE_UINT64);
	*_pntr->amysql_uint64 = tmp;
      }
    }
    break;

  case MYSQL_TYPE_FLOAT:
  case MYSQL_TYPE_DOUBLE:
    {
      double d;
      if ((ret = convertdouble (c, &d))) {
	_pntr->set_typ (AMYSQL_TYPE_DOUBLE);
	*_pntr->amysql_double = c;
      }
    }
    break;

  case MYSQL_TYPE_TIMESTAMP:
  case MYSQL_TYPE_DATE:
  case MYSQL_TYPE_TIME:
  case MYSQL_TYPE_DATETIME:
    {
      okdate_t d;
      d.set (c, global_gmt_offset.get ());
      if ((ret = d.valid ())) {
	_pntr->set_typ (AMYSQL_TYPE_DATE);
	d.to_xdr (_pntr->amysql_date);
      }
    }
    break;

  case MYSQL_TYPE_STRING:
  case MYSQL_TYPE_VAR_STRING:
    {
      str s (c, l);
      if (has_null_byte (s)) {
	_pntr->set_typ (AMYSQL_TYPE_OPAQUE);
	*_pntr->amysql_opaque = s;
      } else {
	_pntr->set_typ (AMYSQL_TYPE_STRING);
	*_pntr->amysql_string = s;
      }
    }
    ret = true;
    break;

  case MYSQL_TYPE_BLOB:
    {
      _pntr->set_typ (AMYSQL_TYPE_OPAQUE);
      *_pntr->amysql_opaque = str (c, l);
      ret = true;
    }
    break;

  case MYSQL_TYPE_NULL:
    _pntr->set_typ (AMYSQL_TYPE_NULL);
    ret = true;
    break;

  default:
    break;
  }

  if (!ret) { _pntr->set_typ (AMYSQL_TYPE_ERROR); }

  return ret;
}

//-----------------------------------------------------------------------

void 
mybind_xdr_union_t::to_qry (MYSQL *m, strbuf *b, char **s, u_int *l)
{
  assert (false);
}

//-----------------------------------------------------------------------

ptr<mybind_t>
mybind_json_t::alloc (const xpub3_json_t &x, bool lenient)
{
  ptr<mybind_t> p;
  switch (x.typ) {
  case XPUB3_JSON_STRING:
    {
      str tmp (opaque2str (*x.json_string));
      p = New refcounted<mybind_str_t> (tmp);
    }
    break;
  case XPUB3_JSON_INT32:
    p = New refcounted<mybind_int_t> (*x.json_int32);
    break;
  case XPUB3_JSON_UINT32:
    p = New refcounted<mybind_int_t> (*x.json_uint32);
    break;
  case XPUB3_JSON_INT64:
    p = New refcounted<mybind_int_t> (*x.json_int64);
    break;
  case XPUB3_JSON_UINT64:
    p = New refcounted<mybind_u64_t> (*x.json_uint64);
    break;
  case XPUB3_JSON_BOOL:
    p = New refcounted<mybind_int_t> (*x.json_bool);
    break;
  case XPUB3_JSON_DOUBLE:
    {
      double d = 0.0;
      if (convertdouble (x.json_double->val, &d) || lenient) {
	p = New refcounted<mybind_double_t> (d);
      }
    }
    break;
  default:
    break;
  }
  return p;
}

//-----------------------------------------------------------------------

str
mybind_json_t::to_str () const
{
  ptr<mybind_t> b = mybind_json_t::alloc (_x, true);
  str ret;
  if (b) ret = b->to_str ();
  return ret;
}

//-----------------------------------------------------------------------

bool
mybind_json_t::read_str (const char *c, unsigned long l, eft_t typ)
{
  bool ret = false;
  switch (typ) {

  case MYSQL_TYPE_TINY:
  case MYSQL_TYPE_SHORT:
  case MYSQL_TYPE_LONG:
  case MYSQL_TYPE_INT24:
  case MYSQL_TYPE_TIMESTAMP:
    {
      int64_t tmp;
      if ((ret = convertint (c, &tmp))) {
	_pntr->set_typ (XPUB3_JSON_INT64);
	*_pntr->json_int64 = tmp;
	ret = true;
      }
    }
    break;

  case MYSQL_TYPE_LONGLONG:
    {
      u_int64_t tmp;
      if ((ret = convertuint (c, &tmp))) {
	_pntr->set_typ (XPUB3_JSON_UINT64);
	*_pntr->json_uint64 = tmp;
	ret = true;
      }
    }
    break;

  case MYSQL_TYPE_FLOAT:
  case MYSQL_TYPE_DOUBLE:
    {
      double d;
      if ((ret = convertdouble (c, &d))) {
	_pntr->set_typ (XPUB3_JSON_DOUBLE);
	_pntr->json_double->val = c;
	ret = true;
      }
    }
    break;

  case MYSQL_TYPE_DATE:
  case MYSQL_TYPE_TIME:
  case MYSQL_TYPE_DATETIME:
  case MYSQL_TYPE_STRING:
  case MYSQL_TYPE_VAR_STRING:
  case MYSQL_TYPE_BLOB:
    _pntr->set_typ (XPUB3_JSON_STRING);
    cstr2opaque (c, l, *_pntr->json_string);
    ret = true;
    break;

  case MYSQL_TYPE_NULL:
    _pntr->set_typ (XPUB3_JSON_NULL);
    ret = true;
    break;

  default:
    break;
  }

  if (!ret) { _pntr->set_typ (XPUB3_JSON_ERROR); }

  return ret;
}

//-----------------------------------------------------------------------

void 
mybind_json_t::to_qry (MYSQL *m, strbuf *b, char **s, u_int *l)
{
  assert (false);
}

//-----------------------------------------------------------------------

#ifdef HAVE_MYSQL_BIND
inline void
mybind_double_t::bind (MYSQL_BIND *bnd, bool param)
{
  bnd->buffer_type = ft;
  bnd->buffer = (char *)&val;
  bnd->buffer_length = sizeof (val);
  bnd->is_null = param ? 0 : &nullfl;
  bnd->length = param ? 0 : &length;
}
#endif

//-----------------------------------------------------------------------

bool
mybind_double_t::read_str (const char *c, unsigned long, eft_t typ)
{
  if (!c) {
    val = 0;
    nullfl = true;
  }
  else {
    char *end;
    val = strtod (c, &end);
    if (end && *end != '\0') {
      val = 0;
      nullfl = true;
    }
  }
  pntr_assign ();
  return true;
}

//-----------------------------------------------------------------------

#endif /* HAVE_MYSQL */
