
// -*-c++-*-
/* $Id$ */

#include "mystmt_ag.h"

#ifndef _LIBAMYSQL_STMTS_H
#define _LIBAMYSQL_STMTS_H

class sth_parsed_t : public mystmt_t 
{
public:
  sth_parsed_t (MYSQL *m, const str &q, u_int o = 0) 
    : mystmt_t (), mysql (m), qry (q), n_iparam (0), bufs (NULL), opts (o),
      myres (NULL) {}
  ~sth_parsed_t ();
  static ptr<sth_parsed_t> alloc (MYSQL *m, const str &q, u_int o)
  { return New refcounted<sth_parsed_t> (m, q, o); }
  bool parse ();
  adb_status_t fetch2 (bool bnd = false);
protected:
  bool execute (MYSQL_BIND *b, mybind_param_t **aarr, u_int n);

  void dealloc_bufs ();
  void alloc_bufs ();
  void row_to_res (MYSQL_ROW *r);

  MYSQL *mysql;
  const str qry;
  u_int n_iparam;
  vec<str> qry_parts;
  char **bufs;
  u_int *lens;
  u_int opts;
  MYSQL_RES *myres;
  unsigned long *length_arr;
  u_int my_res_n;
};

class sth_prepared_t : public mystmt_t 
{
public:
  sth_prepared_t (MYSQL_STMT *s) : mystmt_t (), sth (s), bnds (NULL) {}
  ~sth_prepared_t ();
  static ptr<sth_prepared_t> alloc (MYSQL_STMT *s)
  { return New refcounted<sth_prepared_t> (s); }
  adb_status_t fetch2 (bool bnd = false);
protected:
  bool execute (MYSQL_BIND *b, mybind_param_t **aarr, u_int n);
  void bind (MYSQL_BIND *b, mybind_param_t **arr, u_int n);
  bool bind_result ();
  MYSQL_STMT *sth;
  MYSQL_BIND *bnds;
};


#endif /* _LIBAMYSQL_STMTS_H */
