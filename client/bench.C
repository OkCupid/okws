
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
  warn ("%d iter in %d usec: %f\n", iter, t, (double)t / (double)iter);
}
