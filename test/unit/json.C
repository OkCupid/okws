#include "pub3parse.h"

int
main (int argc, char *argv[])
{
#define BUFLEN 1024
  char buf[1024];
  int rc;
  strbuf b;
  vec<str> v;

  make_sync (0);

  while ((rc = read (0, buf, BUFLEN)) > 0) {
    str s (buf, rc);
    b << s;
    v.push_back (s);
  }

  str tmp = b;

  if (rc < 0) {
    warn ("read error: %m\n");
    return -1;
  }

  for (size_t i = 0; i < 10; i++) {
    ptr<pub3::expr_t> e = pub3::json_parser_t::parse (tmp);
    if (e) {
      str j = e->to_str ();
      warn << "iter " << i << ": " << j << "\n";
    } else {
      warn << "parse failed!\n";
    }
  }
  return 0;
}

#undef BUFLEN
