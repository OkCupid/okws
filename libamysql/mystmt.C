
#include "mystmt.h"

bool
sth_prepared_t::bind_result ()
{
  if (!bnds)
    bnds = New MYSQL_BIND[res_n];
  assert (res_arr);
  for (u_int i = 0; i < res_n; i++)
    res_arr[i].bind (&bnds[i]);
  if (mysql_bind_result (sth, bnds)) {
    err = strbuf ("bind failed: ") << mysql_stmt_error (sth);
	errno_n = mysql_stmt_errno (sth);
    return false;
  }
  return true;
}

void
sth_prepared_t::bind (MYSQL_BIND *b, mybind_param_t **a, u_int n)
{
  for (u_int i = 0; i < n; i++)
    a[i]->bind (&b[i]);
}

void
mystmt_t::alloc_res_arr (u_int n)
{
  res_n = n;
  if (!res_arr)
    res_arr = New mybind_res_t[n];
}

adb_status_t
sth_prepared_t::fetch2 (bool bnd)
{
  if (bnd && !bind_result ())
    return ADB_BIND_ERROR;
  if (mysql_fetch (sth) == MYSQL_NO_DATA) {
	  err = strbuf("fetch error:  ") << mysql_stmt_error (sth);
	  errno_n = mysql_stmt_errno (sth);
	  return ADB_NOT_FOUND;
  }
  assign ();
  return ADB_OK;
}

adb_status_t 
sth_parsed_t::fetch2 (bool bnd)
{
  if (!myres) {
    myres = (opts & AMYSQL_USERES) ? mysql_use_result (mysql) :
      mysql_store_result (mysql);

    if (myres) 
      my_res_n = mysql_num_fields (myres);
    else {
      err = strbuf ("MySQL result error: ") << mysql_error (mysql);
	  errno_n = mysql_errno (mysql);
      return ADB_ERROR;
    }
  }
  MYSQL_ROW row = mysql_fetch_row (myres);
  if (!row) 
    return ADB_NOT_FOUND;
  length_arr = mysql_fetch_lengths (myres);

  row_to_res (&row);
  return ADB_OK;
}

void
sth_parsed_t::row_to_res (MYSQL_ROW *row)
{
  u_int lim = min (my_res_n, res_n);
  for (u_int i = 0; i < lim; i++) {
    res_arr[i].read_str ((*row)[i], length_arr ? length_arr[i] : 0);
  }
}

void
mystmt_t::assign ()
{
  for (u_int i = 0; i < res_n; i++) 
    res_arr[i].assign ();
}

bool
sth_prepared_t::execute (MYSQL_BIND *b, mybind_param_t **arr, u_int n)
{
  if (b && arr && n) {
    bind (b, arr, n);
    if (mysql_bind_param (sth, b)) {
      err = strbuf ("bind error: ") << mysql_stmt_error (sth);
	  errno_n = mysql_stmt_errno (sth);
      return false;
    }
  }
  if (mysql_execute (sth)) {
    err = strbuf ("execute error: ") << mysql_stmt_error (sth);
	errno_n = mysql_stmt_errno (sth);
    return false;
  }
  return true;
}

mystmt_t::~mystmt_t ()
{
  if (res_arr) delete [] res_arr;
}

sth_prepared_t::~sth_prepared_t ()
{
  if (bnds) delete [] bnds;
  if (sth) mysql_stmt_close (sth);
}

sth_parsed_t::~sth_parsed_t ()
{
  if (myres) {
    mysql_free_result (myres);
    myres = NULL;
  }
  dealloc_bufs ();
}

void
sth_parsed_t::alloc_bufs ()
{
  if (!bufs && n_iparam) {
    bufs = New (char *)[n_iparam];
    memset ((void *)bufs, 0, sizeof (char *) * n_iparam);
    lens = New u_int[n_iparam];
    memset ((void *)lens, 0, sizeof (int) * n_iparam);
  }
}

void
sth_parsed_t::dealloc_bufs ()
{
  if (bufs) {
    for (u_int i = 0; i < n_iparam; i++) 
      if (bufs[i]) delete [] bufs[i];
    delete [] bufs;
  }
}

bool
sth_parsed_t::parse ()
{
  const char *p1, *p2;
  p1 = qry.cstr ();
  int len;
  int len_left = qry.len ();
  if (len_left == 0 || p1[0] == '?')
    return false;
    
  while (p1 && *p1 && (p2 = strchr (p1, '?'))) {
    n_iparam++;
    if ((len = p2 - p1) > 0) {
      qry_parts.push_back (str (p1, len));
      p1 = p2 + 1;
    }
    len_left -= (len + 1);
  }
  if (p1 && *p1 && len_left)
    qry_parts.push_back (str (p1, len_left));

  return true;
}

bool
sth_parsed_t::execute (MYSQL_BIND *dummy, mybind_param_t **aarr, u_int n)
{
  if (myres) {
    mysql_free_result (myres);
    myres = NULL;
  }
  if (n != n_iparam) {
	  err = strbuf("cannot prepare query: wrong number of input parameters (n = ") <<
		  n << ", n_iparam = " << n_iparam << ")";
    return false;
  }
  alloc_bufs ();
  strbuf b;
  for (u_int i = 0; i < n; i++) {
    b << qry_parts[i];
    aarr[i]->to_qry (mysql, &b, &bufs[i], &lens[i]);
  }
  for (u_int i = n; i < qry_parts.size (); i++)
    b << qry_parts[i];

  str q = b;
  if (mysql_real_query (mysql, q.cstr (), q.len ())) {
    err = strbuf ("Query execution error: ") << mysql_error (mysql) << "\n";
	errno_n = mysql_errno (mysql);
    return false;
  }
  return true;
}

