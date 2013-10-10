/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
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

#include "pjail.h"
#include <unistd.h>
#include "rxx.h"
#include "okdbg.h"

bool
jailable_t::chroot ()
{
  if (jaildir && !uid && isdir (jaildir)) {
    if (::chroot (jaildir.cstr ()) != 0)
      return false;
    jailed = true;
    return true;
  }
  return false;
}

str
jailable_t::jail2real (const str &exe, bool force) const
{
  // if we're jailed, the we don't really want to have the option
  // of seeing the actual directory, relative to the real root;
  // but, we can fake getting the directory relative to the jail
  // if we give the 'force' flag
  
  str ret;
  if (force || jailed) {
    if (exe[0] == '/')
      ret = exe;
    else {
      strbuf b ("/");
      b << exe;
      ret = b;
    }
  } else {
    if (!jaildir) return exe;
    strbuf b (jaildir);
    if (exe[0] != '/') b << "/";
    b << exe;
    ret = b;
  }

  if (OKDBG2 (OKD_JAIL)) {
    strbuf b;
    b << "jail2real (" << exe << ", " << (force ? "True" : "False")
      << ") -> " << ret << "\n";
    okdbg_warn (CHATTER, b);
  }
  return ret;
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
    warn << "can't call run jail_mkdir unless root\n";
    return false;
  }
  assert (d);
  
  if (!dir_security_check (d)) {
    warn << d << ": directory contains forbidden directory references\n";
    return false;
  }

  // pass false; we haven't chrooted yet
  str dir = jail2real (d);
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
    warn << "made jail directory: " << dir << "\n";
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
jailable_t::fix_stat (const str &d, mode_t mode, ok_usr_t *u, 
		      ok_grp_t *g,
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
    if (!*u) {
      warn ("%s: no such user!\n", u->getname ().cstr ());
      return false;
    }
    else if (u->getid () != newuid) {
      newuid = u->getid ();
      chownit = true;
    }
  }
 
  if (g) {
    if (!*g) {
      warn ("%s: no such group!\n", g->getname ().cstr ());
      return false;
    } else if (g->getid () != newgid) {
      newgid = g->getid ();
      chownit = true;
    }
  }

  if (chownit) {
    if (chown (d.cstr(), newuid, newgid) != 0) {
      warn ("%s: cannot chown: %m\n", d.cstr ());
      return false;
    }
  }

  return true;
}

#define JAIL_CP_BUFSIZE 1024

bool
okws_cp (const str &src, const str &dest, int mode)
{
  bool ret = true;
  char buf[JAIL_CP_BUFSIZE];
  int infd = open (src.cstr (), O_RDONLY);
  if (infd < 0) {
    warn ("%s: cannot access file: %m\n", src.cstr ());
    return false;
  }
  int outfd = open (dest.cstr (), O_CREAT | O_TRUNC | O_WRONLY, mode);
  if (outfd < 0) {
    warn ("%s: cannot copy to file: %m\n", dest.cstr ());
    return false;
  }

  int rc = 0, rc2 = 0;
  while (ret && (rc = read (infd, buf, JAIL_CP_BUFSIZE)) > 0) {
    if ((rc2 = write (outfd, buf, rc)) < 0) {
      warn ("%s: bad write: %m\n", dest.cstr ());
      ret = false;
    } else if (rc2 < rc) {
      warn ("%s: short write: %m\n", dest.cstr ());
      ret = false;
    }
  }
  if (rc < 0) {
    warn ("%s: read error: %m\n", src.cstr ());
    close (outfd);
    return false;
  }

  close (outfd);
  close (infd);
  return ret;
}

bool
jailable_t::jail_cp (const str &fn, mode_t mode, ok_usr_t *u, ok_grp_t *g)
{
  if (!will_jail ())
    return true;

  struct stat sb;

  // false argument --> we haven't chrooted yet.
  str dest = jail2real (fn);
  if (fn == dest) {
    warn << "refusing to copy file to itself: " << dest << "\n";
    return false;
  }
  if (!okws_cp (fn, dest, mode))
    return false;

  if (will_jail ()) {
    if (stat (dest.cstr (), &sb) != 0) {
      warn ("%s: cannot access newly copied file\n", dest.cstr ());
      return false;
    }
    if (!fix_stat (dest, mode, u, g, sb))
      return false;
  }
  return true;
}
