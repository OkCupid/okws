
#include "pubutil.h"

struct A : public virtual refcount {
  A () {}
};

struct B : public A {

};

int
main (int argc, char *argv[])
{
  if (argc != 2)
    exit (1);

  str fn = argv[1];
  phashp_t p = file2hash (fn);
  if (!p)
    fatal << fn << ": cannot access regular file\n";
  warnx << p->to_str () << "\n";
}
