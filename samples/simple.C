
#include "async.h"
#include "arpc.h"
#include "simple_prot.h"
#include "parseopt.h"

int window;
int nalive;
int n;
int nmax;
int nleft;
ptr<axprt_stream> x;
ptr<aclnt> cli;
vec<int> q;

void done (ptr<int> res, clnt_stat err);
void launch (int n);

void
done (ptr<int> res, clnt_stat err)
{
  if (err)
    fatal << "RPC Error: " << err << "\n";
  nalive--;
  if (!--nleft)
    exit (0);
  while (nalive < window && q.size ())
    launch (q.pop_front ());
}

void
launch (int n)
{
  if (nalive >= window) {
    q.push_back (n);
    return;
  }
  ptr<int> res = New refcounted<int> ();
  nalive++;
  cli->call (SIMPLE_SIMPLE, &n, res, wrap (done, res));
}

static void 
connected (int fd)
{
  if (fd < 0)
    fatal << "could not connect to host\n";
  x = axprt_stream::alloc (fd);
  cli = aclnt::alloc (x, simple_prog_1);

  for (n = 0; n < nmax; n++)
    launch (n);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  if (argc != 4 || !convertint (argv[2], &nmax) || 
      !convertint (argv[3], &window))
    fatal << "usage: " << argv[0] << " <host> <num> <window>\n";
  nalive = 0;
  nleft = nmax;
  n = 0;
  tcpconnect (argv[1], SIMPLE_PORT, wrap (connected));
  amain ();
}
