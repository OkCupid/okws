
#include "async.h"

class A {
public:
  A () : v (0) {}
  virtual ~A () {}
  virtual void foo () = 0;
protected:
  u_int64_t v;
};

class B : public A {
public:
  B () : A () {}
  void foo () { v++; }
};



timeval tv;

inline void startt () { gettimeofday (&tv, NULL); }
inline int stopt () 
{ 
  timeval tv2; 
  gettimeofday (&tv2, NULL); 
  return ( (tv2.tv_sec - tv.tv_sec) * 1000000 +
	   (tv2.tv_usec - tv.tv_usec)) ;
}

void
foobar (int *i)
{
  (*i)++;
}

char buf[7*25*25];


static
void foo ()
{
  char *bp = buf;
  for (int i = 0; i < 25; i++) {
    for (int j = 0; j < 25; j++) {
      bp += sprintf (bp, " %d\n", i*j);
    }
  }
  *bp = 0;
}

int 
main (int argc, char *argv[])
{
  for (int k = 0; k <  50000; k++)
    foo ();
}

/*
int
main (int argc, char *argv[])
{
  B b;
  startt ();
  int iter = 1000000;
  for (int i = 0; i < iter; i++) {
    b.foo ();
  }
  int t = stopt ();
  callback<void, int *>::ref cb  = wrap (foobar);
  warn ("virtual function call: %d iter in %d usec\n", iter, t);
  int v = 0;
  startt ();
  for (int i = 0; i < iter; i++) {
    (*cb) (&v);
  }
  t = stopt ();
  warn ("callback call: %d iter in %d usec\n", iter, t);
}
*/
