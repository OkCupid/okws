
#include "pubutil.h"

int
main (int argc, char *argv[])
{
  str s = "hello\nworld\n\n\n";
  str t = "bye world";
  str u = "hello\ndolly\n";

  fix_trailing_newlines (&s);
  fix_trailing_newlines (&t);
  fix_trailing_newlines (&u);

  warn << s << t << u ;
  return 0;
}
