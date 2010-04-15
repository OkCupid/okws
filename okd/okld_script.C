// -*-c++-*-
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

#include "okld.h"

bool
okld_interpreter_t::check (str *err)
{
  if (!_user) {
    *err = strbuf ("user %s was not found", _user.getname ().cstr ());
    return false;
  }

  if (!_group) {
    *err = strbuf ("group %s was not found", _group.getname ().cstr ());
    return false;
  }

  return true;
}

bool
okld_interpreter_t::base_init ()
{
  // set the base interpreter's permissions to 550, so that compromised
  // services cannot access it.
  return get_unix_stat () &&
    fixup (_user.getid (), _group.getid (), ok_interpreter_mode);
}

bool
okld_ch_script_t::fixup_doall (int uid_orig, int uid_new, int gid_orig,
			       int gid_new, int mode_orig)
{
  // need to know where this is to copy it
  okld_interpreter_t *base_ipret = _ipret;
  
  // set the copied interpreter to have this mode
  int newmode = ok_svc_mode;

  _ipret = New okld_interpreter_t (*_ipret, uid_new, gid_new);
  _free_ipret = true;

  if (!_ipret->get_unix_stat ()) {
    str src = base_ipret->get_execpath ();
    str dst = _ipret->get_execpath ();
    if (!okws_cp (src, dst, newmode) || !_ipret->get_unix_stat ()) {
      warn << "Copy interpreter " << src << " to " << dst << " failed!\n";
      return false;
    }
  }

  // set the interpreter
  if (!_ipret->fixup (uid_new, gid_new, newmode))
    return false;

  // set the script
  if (!fixup (uid_new, gid_new, ok_script_mode))
    return false;
  
  return okld_ch_cluster_t::fixup_doall (uid_orig, uid_new, gid_orig,
					 gid_new, mode_orig);
}
