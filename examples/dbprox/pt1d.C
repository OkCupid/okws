#include "pt1_prot.h"
#include "pslave.h"

#include "okwsconf.h"
#ifdef HAVE_MYSQL
#include "amysql.h"
#include "mystmt.h"
#include "web.h"

//typedef callback<void, ptr<t1_user_t> >::ref usercb_t;
class pt1_srv_t : public amysql_thread_t {
public:
 pt1_srv_t (mtd_thread_arg_t *a) 
   : amysql_thread_t (a), err (false) {}
  void dispatch (svccb *sbp);
  bool init ();
  static mtd_thread_t *alloc (mtd_thread_arg_t *arg) 
  { return New pt1_srv_t (arg); }
protected:
  void insert (svccb *sbp);
  void lookup (svccb *sbp);
  adb_status_t lookup (int id, ptr<mystmt_t> sth, mybind_param_t p);
private:
  sth_t q_em, q_nm, q_id, ins;
  sth_t qry;
  bool err;
};

bool
pt1_srv_t::init ()
{
  if (!mysql.connect ("pt1", "root", "localhost")) {
    TWARN (mysql.error ());
    return false;
  }
  if(!(qry = PREP("SELECT * FROM sha1_tab WHERE x = ?")))
     return false;

  return true;
}

void
pt1_srv_t::dispatch (svccb *sbp)
{
  //int id = getid ();
  u_int p = sbp->proc ();
  err = false;
  switch (p) {
  case PT1_NULL:
    replynull ();
    break;
  case PT1_QUERY:
    lookup (sbp);
    break;
  default:
    reject ();
  }
}

void
pt1_srv_t::lookup (svccb *b)
{
  ptr<pt1_query_res_t> u = New refcounted<pt1_query_res_t> (ADB_ERROR);
  int *arg = b->template getarg<int> ();
  str err;
  adb_status_t rc = ADB_OK;
  u->set_status (ADB_OK);
  if(!qry->execute(*arg))
    rc =  ADB_EXECUTE_ERROR;
  else 
    qry->fetch (&u->out->id, &u->out->sha1);
  u->set_status (rc);
  if ((err = qry->error ())) 
    TWARN(err);
  reply (u);
}

#endif  /* HAVE_MYSQL */

int
main (int argc, char *argv[])
{
#ifdef HAVE_MYSQL
  int cnt = 30;
  int maxq = 10000;
  int tmp;
  if (argc > 1 && convertint (argv[1], &tmp))
    cnt = tmp;
  ssrv_t *s = New ssrv_t (wrap (&pt1_srv_t::alloc), pt1_prog_1, MTD_PTH, 
			  cnt, maxq);
  if (!pub_server (wrap (s, &ssrv_t::accept), PT1_PORT))
    fatal << "Cannot bind to port " << PT1_PORT << "\n";
  amain ();
#endif
#ifndef HAVE_MYSQL
  exit (1);
#endif
}
