
#include "t1_prot.h"
#include "pslave.h"

#include "acgiconf.h"
#ifdef HAVE_MYSQL
#include "amysql.h"
#include "mystmt.h"
#include "web.h"

typedef callback<void, ptr<t1_user_t> >::ref usercb_t;
class t1d_srv_t : public amysql_thread_t {
public:
 t1d_srv_t (mtd_thread_arg_t *a) 
   : amysql_thread_t (a), err (false) {}
  void dispatch (svccb *sbp);
  bool init ();
  static mtd_thread_t *alloc (mtd_thread_arg_t *arg) 
  { return New t1d_srv_t (arg); }
protected:
  void insert (svccb *sbp);
  void lookup (svccb *sbp);
  adb_status_t lookup (t1_user_t *u, ptr<mystmt_t> sth, mybind_param_t p);
private:
  sth_t q_em, q_nm, q_id, ins;
  bool err;
};

bool
t1d_srv_t::init ()
{
  if (!mysql.connect ("okd", "root", "okc.maxk.org")) {
    TWARN (mysql.error ());
    return false;
  }
  str parms = "id,name,email,zipcode,yob,sex";
  if (!(q_id = PREP ("SELECT " << parms << " FROM users WHERE id = ?")) ||
      !(q_nm = PREP ("SELECT " << parms << " FROM users WHERE name = ?")) ||
      !(q_em = PREP ("SELECT " << parms << " FROM users WHERE email = ?")) ||
      !(ins  = PREP ("INSERT INTO users (" << parms << 
		     ") VALUES(?,?,?,?,?,?)"))) 
    return false;

  return true;
}

void
t1d_srv_t::dispatch (svccb *sbp)
{
  int id = getid ();
  u_int p = sbp->proc ();
  err = false;
  TWARN ("got connection");
  switch (p) {
  case T1_NULL:
    replynull ();
    break;
  case T1_INSERT:
    insert (sbp);
    break;
  case T1_QUERY:
    lookup (sbp);
    break;
  default:
    reject ();
  }
  TWARN ("replied");
}

void
t1d_srv_t::lookup (svccb *b)
{
  ptr<t1_query_res_t> u = New refcounted<t1_query_res_t> (ADB_ERROR);
  t1_query_arg_t *arg = b->template getarg<t1_query_arg_t> ();
  sth_t q;
  mybind_param_t p;
  switch (arg->typ) {
  case T1_QUERY_ID:
    q = q_id;
    p = *arg->id;
    break;
  case T1_QUERY_NAME:
    q = q_nm;
    p = *arg->name;
    break;
  case T1_QUERY_EMAIL:
    q = q_em;
    p = *arg->email;
    break;
  default:
    break;
  }
  if (q) {
    str err;
    u->set_status (ADB_OK);
    adb_status_t rc = lookup (u->user, q, p);
    u->set_status (rc);
    if ((err = q->error ())) TWARN(err);
  }
  reply (u);
}

adb_status_t
t1d_srv_t::lookup (t1_user_t *u, sth_t sth, mybind_param_t p)
{
  if (!sth->execute (p))
    return ADB_EXECUTE_ERROR;
  return sth->fetch (&u->id, &u->name, &u->email, &u->zipcode, 
		    &u->yob, &u->sex);
}

void
t1d_srv_t::insert (svccb *sbp)
{
  ptr<t1_insert_res_t> r = New refcounted<t1_insert_res_t> (ADB_OK);
  t1_user_t *user = sbp->template getarg<t1_user_t> ();
  t1_user_t eu;
  adb_status_t rc;
  str err;

  switch ((rc = lookup (&eu, q_em, user->email))) {
  case ADB_OK:
    rc = ADB_EXISTS;
    break;
  case ADB_NOT_FOUND:
    {
      if (!ins->execute (int (0), user->name, user->email, user->zipcode,
			 user->yob, user->sex)) {
	TWARN (ins->error ());
	rc = ADB_EXECUTE_ERROR;
      } else {
	rc = ADB_OK;
	*r->id = mysql.insert_id ();
      }
    }
    break;
  default:
    if ((err = q_em->error ()))
      TWARN (err);
    break;
  }
  if (rc != ADB_OK)
    r->set_status (rc);
  reply (r);
  return;
}

#endif  /* HAVE_MYSQL */

int
main (int argc, char *argv[])
{
#ifdef HAVE_MYSQL
  ssrv_t *s = New ssrv_t (wrap (&t1d_srv_t::alloc), t1_prog_1, MTD_PTH, 3);
  if (!pub_server (wrap (s, &ssrv_t::accept), T1D_PORT))
    fatal << "Cannot bind to port " << T1D_PORT << "\n";
  amain ();
#endif
#ifndef HAVE_MYSQL
  exit (1);
#endif
}
