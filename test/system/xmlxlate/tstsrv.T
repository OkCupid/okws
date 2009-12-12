
// -*-c++-*-
/* $Id: tstsrv.T 2718 2007-04-15 13:45:19Z max $ */

#include "async.h"
#include "arpc.h"
#include "okws_sfs.h"
#include "parseopt.h"
#include "tame.h"
#include "tame_rpcserver.h"
#include "tstprot.h"

class tstsrv_t : public tame::server_t {
public:
  tstsrv_t (int fd, int v) : tame::server_t (fd, v) {}
  const rpc_program &get_prog () const { return tst_prog_1; }
  void dispatch (svccb *sbp);
};

class tstsrv_factory_t : public tame::server_factory_t {
public:
  tstsrv_factory_t () : tame::server_factory_t () {}
  tame::server_t *alloc_server (int fd, int v) { return New tstsrv_t (fd, v); }
};

void
tstsrv_t::dispatch (svccb *sbp)
{
  assert (sbp);

  u_int p = sbp->proc ();
  switch (p) {
  case TST_NULL:
    sbp->reply (NULL);
    break;
  case TST_RPC1:
    {
      tst_arg_t *arg = sbp->Xtmpl getarg<tst_arg_t> ();
      tst_res_t res;

      res.b = arg->x;
      for (const char *cp = arg->y.cstr (); *cp; cp++) {
	res.v.push_back (int (*cp));
      }
      sbp->replyref (res);
      break;
    }
  case TST_RPC2:
    {
      yy_t *arg = sbp->Xtmpl getarg<yy_t> ();
      ww_t res;

      res.a[0] = *arg;
      res.a[1] = *arg;
      res.v.push_back (*arg);
      res.z = "footime";
      sbp->replyref (res);
      break;
    }
  case TST_RPC3:
    {
      aa_t *arg = sbp->Xtmpl getarg<aa_t> ();
      sbp->reply (arg);
      break;
    }
  case TST_RPC4:
    {
      bb_t *arg = sbp->Xtmpl getarg<bb_t> ();
      sbp->reply (arg);
      break;
    }
  case TST_RPC8:
    {
      long_arg_t *la = sbp->Xtmpl getarg<long_arg_t> ();
      sbp->replyref (la->x + la->opq.size ());
      break;
    }
  case TST_RPC9:
    {
      aston_t *a = sbp->Xtmpl getarg<aston_t> ();
      sbp->replyref (a->questions.size ()); 
      break;
    }
  case TST_RPC10:
    {
      const rpc_10_arg_t *a = sbp->Xtmpl getarg<rpc_10_arg_t> ();
      cc_t res;
      size_t n = a->iters;
      for (size_t i = 0; i < n; i++) {
	res.push_back (a->bb);
      }
      sbp->replyref (res);
    }
    break;
  case TST_RPC11:
    {
      const match_arg_t *a = sbp->Xtmpl getarg<match_arg_t> ();
      u_int64_t o = a->h1;
      if (a->sub) { o-= a->h2; }
      else { o += a->h2; }
      sbp->replyref (o);
    }
    break;
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    bool ret;
    tstsrv_factory_t fact;
  }
  if (argc != 2)
    fatal << "usage: tstsrv <port>\n";

  twait { fact.run (argv[1], mkevent (ret)); }
  exit (ret ? 0 : -1);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
}
