/* $Id$ */

/*
 *
 * Copyright (C) 2003-4 by Maxwell Krohn (max@okcupid.com)
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

bool
rpc_vers::has_positive_toks () const
{
  if (rpc_txa_base_t::has_positive_toks ())
    return true;
  for (u_int i = 0; i < procs.size () ; i ++) {
    if (procs[i].has_positive_toks ())
      return true;
  }
  return false;
}


static void
tokdump2 (str prfx, const str &s, bool b)
{
  aout << prfx << "insert (" << s << ", " << (b ? "true" : "false") << ");\n" ;
}

static void
tokdump1 (str prfx, const str &s, bool *b)
{
  tokdump2 (prfx, s, *b);
}

void
ptxa_tokset_t::dump (const str &prfx) const
{
  trav (wrap (tokdump1, prfx));
}

void
ptxa_tokset_t::trav (callback<void, const str &, bool *>::ref cb) const
{
  qhash<str, bool> *tp = const_cast<qhash<str, bool> *> (&tokens);
  tp->traverse (cb);
} 

void
rpc_proc::dump (const str &prfx) const
{
  if (tokset)
    tokset->dump (str (strbuf (prfx) << "[" << id << "]."));
}

void
rpc_vers::tokdump (const str &s, bool *b) const
{
  if (!tokset || tokset->access (s) != -1)
    tokdump2 ("    ", s, true);
}

void
rpc_vers::dump (const rpc_program_p *prog) const
{
  str rpcstr = rpcprog (prog, this);

  str basename = "txa_prog_t";
  str classname = strbuf (rpcstr) << "_txa_t";
  aout << "\nclass " << classname << " : public " << basename << " {\n"
       << "public:\n"
       << "  " << classname << " () : " << basename << " () {\n";

  if (prog->tokset) 
    prog->tokset->trav (wrap (this, &rpc_vers::tokdump));
  if (tokset)
    tokset->dump ("    ");

  for (u_int i = 0; i < procs.size (); i++) 
    procs[i].dump ("    (*this)");
  if (txa_rpc) {
    aout << "    set_login_rpc (" << txa_rpc << " );\n";
  }

  aout << "  }\n"
       << "};\n";
}

void
rpc_program_p::dump () const
{
  for (u_int i = 0; i < vers.size (); i++) 
    vers[i].dump (this);
}
