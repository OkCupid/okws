
#include <stdio.h>

int fact (int n)
{
  int r = 1;
  for ( ; n > 0 ; n --) {
    printf ("foo %d\n", n);
    r *= n;
  }
  return r;
}
