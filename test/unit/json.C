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

  pub3::obj_t o;

  for (size_t i = 0; i < 10; i++) {
    ptr<pub3::expr_t> e = pub3::json_parser_t::parse (tmp);
    if (e) {
      o = pub3::obj_t (e);
      str j = o.to_str ();
      warn << "iter " << i << ": " << j << "\n";
    } else {
      warn << "parse failed!\n";
    }
  }

  
  for (int i = 1; i < argc; i++) {
    ptr<pub3::expr_t> e = pub3::json_parser_t::parse (argv[i]);
    if (e) {
      pub3::obj_t o2 (e);
      o.append (o2);
    } else {
      warn << "parse failed on arg: " << argv[i] << "\n";
    }
  }

  tmp = o.to_str ();
  warn << "combined: " << tmp << "\n";

  return 0;
}

#undef BUFLEN
