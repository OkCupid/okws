
#include "crypt.h"
#include <sys/time.h>
#include "parseopt.h"

#include "resp.h"
#include <stdio.h>
#include "zstr.h"
#include "sysconf.h"

#include "cgi.h"

/*
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
  u_int sz;
  if (argc != 2 || !convertint (argv[1], &sz)) {
    fprintf (stderr, "usage: tst <size>\n");
    exit (1);
  }
  random_init ();
  bigint j;
  u_int64_t tot = 0;
  int k = 3000;
  int l = 500;
  for (int x = 0; x < l; x++) {
    bigint n = random_bigint (sz);
    bigint m = random_bigint (sz);
    startt ();
    for (int i = 0; i < k ; i++) {
      j = m * n;
    }
    tot += stopt ();
  }
  double d = (double)tot / (double) (k*l);
  printf ("%g\n", d);
}

class A {
public:
  A () : i (0) {}
  A (int ii) : i (ii) {}
  virtual void bar () { printf ("%d\n", i); }
protected:
  const int i;
};

class B1 : public A {
public:
  B1 (int i) : A (i) {}
};

class B2 : public virtual A {
public:
  B2 () : A () {}
};

class C : public B1, virtual public B2 
{
public:
  C (int i) : B1 (i) {}
};

class D : public B1
{
public:
  D (int i) : B1 (i) {}
};


static void
main2 ()
{
  warn << expire_in (1, 0, 0, 0) << "\n";
}


class A {
public:
  A () : bp (buf) {} 
  const A &cat (int i) const 
  { bp += sprintf (bp, "%d", i); return (*this); }
  const A &cat (const char *s) const 
  { 
    u_int l = strlen (s);
    memcpy (bp, s, l); 
    bp += l;
    return (*this);
  }
  void output () { *bp = 0; printf ("%s\n", buf); }
  mutable char *bp;
  char buf[200];
};

const A &operator<< (const A &a, const char *b) { return a.cat (b); }
const A &operator<< (const A &a, int i) { return a.cat (i); }

int
main (int argc, char *argv[]) 
{
  zinit ();
  strbuf b;
  for (u_int i = 0; i < 300; i++) {
    b << "I am " << i << " years old.\n";
  }
  str s ("droote + jabbres!");
  str s2 = b;
  zbuf z;
  const char *s3 = " dah dah dah\n";
  z << s2 << s << s2 << "\n and that is that!\n";
  z << s3 << s << s3;
  const strbuf &out = z.compress (9);
  warn << out;
}

*/

template<typename T>
class B {
public:
  B (ptr<T> *t) : p (t) {}
  void foo (int i) { *p = New refcounted<T> (i); }
private:
  ptr<T> *p;
};

class C : public B<int>
{
public:
  C (ptr<int> *i) : B<int> (i) {}
};

typedef unsigned long u64_t;
static void foo () { ptr<u64_t> t = New refcounted<u64_t> (); }

int
main (int argc, char *argv)
{
  str s = "hello\nmax\r\nis\nmy\name";
  str s2 = cgi_encode (s);
  warn << "s: " << s;
  warn << "s2: " << s2 << "\n";
  warn << "decoded: " << cgi_decode (s2) << "\n";

  str s3 = "max+krohn+is+here+now%aeto%%introduce%pp%a0some bugs%";
  warn << "s3: " << s3 << "\n\n";
  warn << "decoded: " << cgi_decode (s3) << "\n";

  str b = "max & blah + blah \" ";
  warn << cgi_decode (b) << "\n";
}
