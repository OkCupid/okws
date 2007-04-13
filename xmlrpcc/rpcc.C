/* $Id: rpcc.C 930 2005-07-15 04:59:36Z max $ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include "rpcc.h"

str idprefix;
bhash<str> ids;

const str shell ("/bin/sh");
static str outfile;

str
stripfname (str fname, bool suffix)
{
  const char *p;

  if ((p = strrchr (fname, '/')))
    p++;
  else p = fname;

  if (suffix)
    return p;

  const char *ep = p;
  while (*ep && *ep != '.')
    ep++;

  return str (p, ep - p);
}

str
rpcprog (const rpc_program *rp, const rpc_vers *rv)
{
  strbuf name;

  for (const char *p = rp->id.cstr (); *p; p++)
    name << char (tolower (*p));
  return name << "_" << rv->val;
}

pid_t
runcpp (char **argv)
{
  pid_t pid;
  int fds[2];

  if (pipe (fds) < 0)
    fatal ("pipe: %m\n");

  pid = fork ();
  if (pid == -1)
    fatal ("fork: %m\n");

  if (!pid) {
    close (fds[0]);
    if (fds[1] != 1) {
      dup2 (fds[1], 1);
      close (fds[1]);
    }
    execv (PATH_CPP, argv);
    fatal ("%s: %m\n", PATH_CPP);
  }

  close (fds[1]);
  if (fds[0] != 0) {
    dup2 (fds[0], 0);
    close (fds[0]);
  }
  return pid;
}

static void
setstdout ()
{
  if (outfile[0] != '|') {
    int fd;
    unlink (outfile);
    if ((fd = open (outfile, O_CREAT|O_WRONLY, 0666)) < 0) {
      perror (outfile);
      exit (1);
    }
    if (fd != 1) {
      dup2 (fd, 1);
      close (fd);
    }
  }
  else {
    int fds[2];
    if (pipe (fds) < 0) {
      perror ("pipe");
      exit (1);
    }
    // pid_t pid = afork ();
    pid_t pid = fork ();
    if (pid < 0) {
      perror ("fork");
      exit (1);
    }
    if (!pid) {
      close (fds[0]);
      dup2 (fds[1], 0);
      if (fds[1])
        close (fds[1]);
      execl (shell, shell, "-c", outfile.cstr () + 1, (char *) NULL);
      exit (1);
    }
    close (fds[1]);
    if (fds[0] != 1) {
      dup2 (fds[0], 1);
      close (fds[0]);
    }
  }
}

static void
cleanup ()
{
  if (outfile)
    unlink (outfile);
}

static void
usage ()
{
  warn << "usage: rpcc {-c | -h }\n"
    "            [-Ppref] [-Ddef] [-Idir]\n\t[-o outfile] file.x\n";
  exit (1);
}

void
reapcpp (int status)
{
  if (status) {
    warn ("cpp failed (status 0x%x)\n", status);
    exit (1);
  }
  outfile = NULL;
  exit (0);
}

int
main (int argc, char **argv)
{
  pid_t child;
  int an;
  vec<char *> av;
  char *fname = NULL;
  char *basename;
  enum { BAD, HEADER, CFILE } mode = BAD;
  void (*fn) (str, str) = NULL;
  int len;

  av.push_back (PATH_CPP);
  av.push_back ("-DRPCC");
  av.push_back (NULL);

  for (an = 1; an < argc; an++) {
    char *arg = argv[an];
    int arglen = strlen (arg);

    if (arg[0] == '-' && (arg[1] == 'D' || arg[1] == 'I'))
      av.push_back (arg);
    else if (!fname && arglen > 2 && arg[0] != '-'
	     && arg[arglen-1] == 'x' && arg[arglen-2] == '.')
      fname = arg;
    else if (!strcmp (arg, "-h") && mode == BAD)
      mode = HEADER;
    else if (!strcmp (arg, "-c") && mode == BAD)
      mode = CFILE;
    else if (!strcmp (arg, "-o") && !outfile && ++an < argc)
      outfile = argv[an];
    else if (!strncmp (arg, "-o", 2) && !outfile && arg[2])
      outfile = arg + 2;
    else if (!strcmp (arg, "-P") && !idprefix && ++an < argc)
      idprefix = argv[an];
    else if (!strncmp (arg, "-P", 2) && !idprefix && arg[2])
      idprefix = arg + 2;
    else 
      usage ();
  }

  if (!fname)
    usage ();

  if (idprefix)
    idprefix = idprefix << "_";

  av.push_back (fname);
  av.push_back (NULL);

  if ((basename = strrchr (fname, '/')))
    basename++;
  else
    basename = fname;
  len = strlen (basename);

  str xdr_headername = strbuf("%.*sh", len - 1, basename);

  switch (mode) {
  case HEADER:
    av[2] = "-DRPCC_H";
    fn = genheader;
    if (!outfile) {
      outfile = strbuf ("%.*s_xml.h", len - 2, basename);
    }
    break;
  case CFILE:
    av[2] = "-DRPCC_C";
    fn = gencfile;
    if (!outfile)
      outfile = strbuf ("%.*s_xml.C", len - 2, basename);
    break;
  default:
    usage ();
    break;
  }

  child = runcpp (av.base ());

  if (outfile != "-") {
    if (outfile[0] != '|')
      atexit (cleanup);
    setstdout ();
  }

  make_sync (0);
  yyparse ();
  checkliterals ();
  if (outfile != "-" && outfile[0] != '|')
    fn (outfile, xdr_headername);
  else
    fn (fname, xdr_headername);
#if 0
  chldcb (child, wrap (reapcpp));
  amain ();
#else
  int status;
  if (waitpid (child, &status, 0) < 0)
    fatal ("waitpid: %m\n");
  reapcpp (status);
#endif
  return 0;
}
