// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Maxwell Krohn (max@okcupid.com)
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

#ifndef _LIBPUB_PJAIL_H
#define _LIBPUB_PJAIL_H 1

#include "arpc.h"
#include "async.h"
#include "pub3prot.h"
#include <sys/stat.h>
#include "pubutil.h"

class jailable_t {
public:
  jailable_t (const str &j = NULL, int uid = -1) : 
    jaildir (j), jailed (false), uid (uid >= 0 ? uid : getuid ()) {}
  bool chroot ();
  str jail2real (const str &fn, bool force = false) const;
  str nest_jails (const str &path) const;
  bool will_jail () const { return (jailed || (jaildir && !uid)); }
  bool jail_mkdir (const str &d, mode_t mode, ok_usr_t *u, ok_grp_t *g);
  bool jail_mkdir_p (const str &d, mode_t mode, ok_usr_t *u, ok_grp_t *g);
  bool jail_cp (const str &fn, mode_t  mode, ok_usr_t *u, ok_grp_t *g);
  bool is_superuser () const { return uid == 0; }
  int get_uid () const { return uid; }
protected:
  bool fix_stat (const str &d, mode_t mode, ok_usr_t *u, ok_grp_t *g,
		 const struct stat &sb);
  str jaildir;
  bool jailed;
  int uid;
};

bool okws_cp (const str &src, const str &dest, int mode);

#endif /* _LIBPUB_PJAIL_H */
