
#include <normdist.h>
#include <math.h>

static int iround (double d)
{
  int i = int (d);
  if (d - i >= .5) i++;
  return i;
}

void
normdist_t::construct ()
{
  double *p = new double[prec];
  F = new double[prec];
  arr = new int[arrsz];

  double coef = M_2_SQRTPI / (2 * stddv);

  for (int i = 0; i < prec; i++) {
    double x = (i - mean) / stddv;
    p[i] = coef * exp ( -1 * x * x);
  }

  // make CDF
  for (int i = 0; i < prec; i++) F[i] = 0;
  for (int i = 0; i < prec; i++) {
    for (int j = i; j < prec; j++) {
      F[j] += p[i];
    }
  }

  // create uniformly samplable equivalent
  for (int i = 0; i < arrsz; i++) arr[i] = 1;
  for (int i = 0; i < prec; i++) {
    int k = iround (F[i] * arrsz);
    for (int j = k; j < arrsz; j++)
      arr[j] ++;
  }

  // chop off end
  for (e = arrsz - 1; e >= 0 && arr[e] > prec; e--);

  delete [] p;
}


