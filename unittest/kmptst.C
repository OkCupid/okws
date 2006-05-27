
#include "kmp.h"

 
int
main (int argc, char *argv[])
{
  if (argc != 3) 
    fatal << "usage: kmptst <needle> <haystack>\n";

  kmp_matcher_t m (argv[1]);
  int l = strlen (argv[1]);

  int i = 0;
  for (char *cp = argv[2]; *cp; cp++) {
    if (m.match (*cp)) {
      char c = cp[1];
      cp[1] = 0;
      warn << i << "\t" << (cp + 1 - l) << "\n";
      cp[1] = c;
    }
    i++;
  }
}
