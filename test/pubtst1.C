
/* $Id$ */

#include "pub.h"
#include "pub_parse.h"

static void
usage ()
{
  fatal << "usage: pubtst1 <file>.g\n";
}

int
main (int argc, char *argv[])
{
  const pbinding_t *i;
  bpfcp_t bpf;
  pfile_t *f = NULL;
  pub_parser_t *p = pub_parser_t::alloc ();

#ifdef PDEBUG
  yydebug = 1;
#endif /* PDEBUG */

  if (argc != 2) 
    usage ();
  str infile = argv[1];
  str outfile = suffix_sub (infile, ".g", ".C");
  if (!outfile)
    usage ();

  int fd = myopen (outfile);
  if (fd < 0) 
    exit (1);
  int rc = 0;
  if ((i = p->to_binding (infile)) && (bpf = p->parse (i, PFILE_TYPE_CODE))) {

    if (p->parseerr)
      rc = -1;

#ifdef PDEBUG
    dumper_t d;
    bpf->dump (&d);
#endif /* PDEBUG */

    strbuf b;
    if (p->include (&b, bpf, P_COMPILE | P_IINFO)) {
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
