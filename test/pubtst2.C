
/* $Id$ */

#include "pub.h"
#include "pub_parse.h"

static void
usage ()
{
  fatal << "usage: pubtst1 <infile>.shtml [<outfile>]\n";
}


int
main (int argc, char *argv[])
{
  const pbinding_t *i;
  bpfcp_t bnd;
  pub_parser_t *p = pub_parser_t::alloc ();

#ifdef PDEBUG
  yydebug = 1;
#endif /* PDEBUG */

  str infile, outfile;
  if (argc == 2) {
    infile = argv[1];
    outfile = suffix_sub (infile, ".shtml", ".html");
    if (!outfile)
      usage ();
  } else if (argc == 3) {
    infile = argv[1];
    outfile = argv[2];
  } else {
    usage ();
  }

  int fd = myopen (outfile);
  if (fd < 0) 
    exit (1);
  int rc = 0;
  if ((i = p->to_binding (infile)) && (bnd = p->parse (i, PFILE_TYPE_H))) {

    if (p->parseerr)
      rc = -1;

#ifdef PDEBUG
    dumper_t d;
    bnd->dump (&d);
#endif /* PDEBUG */

    strbuf b;
    if (p->include (&b, bnd, P_IINFO)) {
      if (b.tosuio ()->output (fd) < 0) {
	close (fd);
	rc = -1;
      }
    } else {
      rc = -1;
    }
    
  }
  close (fd);
  if (rc < 0)
    unlink (outfile);
  exit (rc);
}
