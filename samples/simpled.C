
#include "simple_prot.h"
#include "amt.h"
#include "pslave.h"

class simple_srv_t : public mtd_thread_t {
public:
  simple_srv_t (mtd_thread_arg_t *a) : mtd_thread_t (a) {}
  void dispatch (svccb *sbp);
  static mtd_thread_t *alloc (mtd_thread_arg_t *arg) 
  { return New simple_srv_t (arg); }
};

int
main (int argc, char *argv[])
{
  ssrv_t *s = New ssrv_t (wrap (&simple_srv_t::alloc), simple_prog_1, 
			  MTD_PTH, 20);
  if (!pub_server (wrap (s, &ssrv_t::accept), SIMPLE_PORT))
    fatal << "Cannot bind to port " << SIMPLE_PORT << "\n";
  amain ();
}

void
simple_srv_t::dispatch (svccb *sbp)
{
  int id = getid ();
  u_int p = sbp->proc ();
  warn << "Thread " << id << ": got connection; sleeping...\n";
  sleep (1);
  warn << "Thread " << id << ": woke up.\n";
  switch (p) {
  case SIMPLE_NULL:
    replynull ();
    break;
  case SIMPLE_SIMPLE:
    {
      simple_xdr_t *in = sbp->template getarg<simple_xdr_t> ();
      ptr<simple_xdr_t> r = New refcounted<simple_xdr_t> ();
      r->i = ++ in->i;
      reply (r);
    }
    break;
  default:
    reject ();
  }
  warn << "Thread " << id << ": replied\n";
}


