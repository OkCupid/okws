
// -*-c++-*-
/* $Id$ */

#include <stdlib.h>
#include <stdio.h>

#define SAMPLE_FACTOR 100

class normdist_t {
public:
  normdist_t (double m, double v)
    : mean (m), stddv (v), prec (int (m*2)), arr (NULL), F (NULL),
      arrsz (prec * prec)
  { construct (); }

  ~normdist_t () 
  { 
    if (arr) delete [] arr; 
    if (F) delete [] F;
  }

  void construct ();
  inline int sample () { return arr[random () % e]; }
  void dump () 
  {
    for (int i = 0; i < e; i++) {
      printf ("%d\n", arr[i]);
    }
  }
    
    

private:
  double mean;
  double stddv;
  int prec;
  int *arr;
  double *F;
  int arrsz;
  int e; // end of sampling
};
