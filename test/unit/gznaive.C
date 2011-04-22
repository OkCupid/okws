#include "ok.h"

static void
try1 (str d)
{
  zbuf z;
  z.cat (d);
  strbuf b;
  z.to_strbuf (&b, true);
  warn << b << "\n";
}


static void
try2 (str d)
{
  zbuf z;
  z.cat (d);
  strbuf b;
  z.naive_compress (&b, 6);
  warn << b << "\n";
}

int
main (int argc, char *argv[])
{
  if (argc != 2) {
    warn << "need a file to try\n";
    return 1;
  }
  str dat = file2str (argv[1]);
  if (!dat) {
    warn << "cannot open file: " << argv[1] << "\n";
    return 1;
  }
  try1 (dat);
  try2 (dat);
  return 0;
}
