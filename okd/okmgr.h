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

#ifndef _OKD_OKMGR_H
#define _OKD_OKMGR_H

#include "okprot.h"
#include "ok.h"
#include "okerr.h"
#include "vec.h"
#include "arpc.h"

typedef enum { CTL_MODE_PUB = 0, CTL_MODE_LAUNCH = 1,
	       CTL_MODE_LOGTURN = 2 } ctl_mode_t;

class okmgr_host_t {
public:
  okmgr_host_t (const str &h, u_int p) : hostname (h), port (p) {}
  void connect (cbb cb);
  ptr<aclnt> cli () const { return c; }
  void hostwarn (const str &s, bool nl = true) const;
private:
  void connect_cb (cbb c, int i);
  const str hostname;
  const u_int port;
  int fd;
  ptr<axprt> x;
  ptr<aclnt> c;
};

class okmgr_clnt_t {
public:
  okmgr_clnt_t (const vec<str> &h);
  virtual ~okmgr_clnt_t () {}
  void run ();
  virtual void do_host (okmgr_host_t *h, ptr<ok_xstatus_t> s) = 0;
  void did_host (okmgr_host_t *h, ptr<ok_xstatus_t> s, clnt_stat err);
  bool add_host (const str &h);
  void finish (bool r);
  void connect_cb (u_int i, bool ok);
private:
  bool err;
  u_int ncli;
  vec<okmgr_host_t *> hosts;
};

class okmgr_pub_t : public okmgr_clnt_t {
public:
  okmgr_pub_t (const vec<str> &h, const vec<str> &f, int v);
  void do_host (okmgr_host_t *h, ptr<ok_xstatus_t> s);
private:
  xpub_fnset_t fns;
  int version;
};

class okmgr_logturn_t : public okmgr_clnt_t {
public:
  okmgr_logturn_t (const vec<str> &h) ;
  void do_host (okmgr_host_t *h, ptr<ok_xstatus_t> x);
};

class okmgr_launch_t : public okmgr_clnt_t {
public:
  okmgr_launch_t (const vec<str> &h, const vec<str> &f, 
		  ok_set_typ_t t = OK_SET_SOME);
  void do_host (okmgr_host_t *h, ptr<ok_xstatus_t> s);
private:
  ok_progs_t progs;
};


#endif /* _OKD_OKMGR_H */
