
#include "pjail.h"
#include <unistd.h>
#include "rxx.h"

bool
jailable_t::chroot ()
{
  if (jaildir && !uid && isdir (jaildir)) {
    ::chroot (jaildir.cstr ());
    jailed = true;
    return true;
  }
  return false;
}

str
jailable_t::jail2real (const str &exe, bool chrt) const
{
  if (chrt && ((!uid && jaildir) || jailed)) 
    if (exe[0] == '/')
      return exe;
    else 
      return strbuf ("/") << exe;
  else {
    if (!jaildir) return exe;
    strbuf b (jaildir);
    if (exe[0] != '/') b << "/";
    b << exe;
    return b;
  }
}

str 
jailable_t::nest_jails (const str &path) const
{
  if (!path)
    return jaildir;
  else if (jailed)
    return path;
  else {
    strbuf b (jaildir);
    if (path[0] != '/') b << "/";
    b << path;
    return b;
  }
}

bool
dir_security_check (const str &dir)
{
  const char *p = dir.cstr ();
  while (*p == '/') { p++; }
  if (strcmp (p, ".") == 0 || strcmp (p, "..") == 0)
    return false;
  return true;
}

bool
jailable_t::jail_mkdir (const str &d, mode_t mode, ok_usr_t *u, ok_grp_t *g)
{
  if (uid) {
    warn << "can't call run jail_mkdir as root\n";
    return false;
  }
  assert (d);
  
  if (!dir_security_check (d)) {
    warn << d << ": directory contains forbidden directory references\n";
    return false;
  }

  // pass false; we haven't chrooted yet
  str dir = jail2real (d, false);
  struct stat sb;
  if (stat (dir.cstr (), &sb) != 0) {
    if (mkdir (dir.cstr (), mode != 0)) {
      warn ("%s: make directory failed: %m\n", dir.cstr ());
      return false;
    } 
    if (stat (dir.cstr (), &sb) != 0) {
      warn << dir 
	   << ": could not STAT directory even though mkdir succeeded.\n";
      return false;
    }
  }
  if (!S_ISDIR (sb.st_mode)) {
    warn << dir << ": file exists and is not a directory\n";
    return false;
  }
  if (!fix_stat (dir, mode, u, g, sb)) {
    warn << dir << ": could not fix up permissions and ownership\n";
    return false;
  }
  return true;
}

static rxx slash ("/+");
bool
jailable_t::jail_mkdir_p (const str &d, mode_t mode, ok_usr_t *u, ok_grp_t *g)
{
  vec<str> subdirs;
  int rc;
  if ((rc = split (&subdirs, slash, d, 32)) <= 0) {
    warn << d << ": malformed directory\n";
    return false;
  }
  strbuf b;
  if (d[0] == '/')
    b << "/";
  for (u_int i = 0; i < subdirs.size (); i++) {
    if (subdirs[i].len ()) {
      b << subdirs[i];
      if (!jail_mkdir (str (b), mode, u, g)) 
	return false;
      b << "/";
    }
  }
  return true;
}

bool
jailable_t::fix_stat (const str &d, mode_t mode, ok_usr_t *u, ok_grp_t *g,
		      const struct stat &sb)
{
  if (uid)
    return false;

  int newgid, newuid;
  bool chownit = false;

  newgid = sb.st_gid;
  newuid = sb.st_uid;

  // do not allow setuid/setgid directories or files
  mode = (mode & 0777);

  if ((sb.st_mode & 0777) != mode) {
    if (chmod (d.cstr (), mode) != 0) {
      warn ("%s: chmod failed: %m\n", d.cstr ());
      return false;
    }
  }

  if (u) {
    if (!*u)
      return false;
    else if (u->getid () != newuid) {
      newuid = u->getid ();
      chownit = true;
    }
  }
 
  if (g) {
    if (!*g)
      return false;
    else if (g->getid () != newgid) {
      newgid = g->getid ();
      chownit = true;
    }
  }

  if (chownit) {
    if (chown (d, newuid, newgid) != 0) {
      warn ("%s: cannot chown: %m\n", d.cstr ());
      return false;
    }
  }

  return true;
}

#define JAIL_CP_BUFSIZE 1024

bool
jailable_t::jail_cp (const str &fn, mode_t mode, ok_usr_t *u, ok_grp_t *g)
{
  if (!will_jail ())
    return true;

  char buf[JAIL_CP_BUFSIZE];
  struct stat sb;

  // false argument --> we haven't chrooted yet.
  str dest = jail2real (fn, false);
  if (fn == dest) {
    warn << "refusing to copy file to itself: " << dest << "\n";
    return false;
  }
  int infd = open (fn.cstr (), O_RDONLY);
  if (infd < 0) {
    warn ("%s: cannot access file: %m\n", fn.cstr ());
    return false;
  }
  int outfd = open (dest.cstr (), O_CREAT | O_TRUNC | O_WRONLY, mode);
  if (outfd < 0) {
    warn ("%s: cannot copy to file: %m\n", dest.cstr ());
    return false;
  }

  int rc, rc2;
  while ((rc = read (infd, buf, JAIL_CP_BUFSIZE)) > 0) {
    if ((rc2 = write (outfd, buf, rc)) < 0) {
      warn ("%s: bad write: %m\n", dest.cstr ());
      goto write_error;
    } else if (rc2 < rc) {
      warn ("%s: short write: %m\n", dest.cstr ());
      goto write_error;
    }
  }
  if (rc < 0) {
    warn ("%s: read error: %m\n", fn.cstr ());
    close (outfd);
    return false;
  }

  close (outfd);
  close (infd);
  
  if (will_jail ()) {
    if (stat (dest.cstr (), &sb) != 0) {
      warn ("%s: cannot access newly copied file\n", dest.cstr ());
      return false;
    }
    if (!fix_stat (dest, mode, u, g, sb))
      return false;
  }
  return true;
    
 write_error:
  close (outfd);
  close (infd);
  return (false);
}
