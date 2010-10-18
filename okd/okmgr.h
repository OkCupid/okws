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
#include "vec.h"
#include "arpc.h"
#include "tame.h"

//-----------------------------------------------------------------------

typedef enum { CTL_MODE_PUB = 0, 
	       CTL_MODE_LAUNCH = 1,
	       CTL_MODE_LOGTURN = 2,
	       CTL_MODE_DIAGNOSTIC = 3,
	       CTL_MODE_SEND_MSG = 4 } ctl_mode_t;

//-----------------------------------------------------------------------

class okmgr_clnt_t {
public:
  okmgr_clnt_t (const str &s);
  virtual ~okmgr_clnt_t () {}
  void run (CLOSURE);
  virtual void do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb) = 0;
private:
  bool _err;
  const str _sockname;
};

//-----------------------------------------------------------------------

class okmgr_logturn_t : public okmgr_clnt_t {
public:
  okmgr_logturn_t (const str &s ) ;
  void do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb);
};

//-----------------------------------------------------------------------

class okmgr_launch_t : public okmgr_clnt_t {
public:
  okmgr_launch_t (const str &s, oksvc_procs_t p, ok_set_typ_t t = OK_SET_SOME);
  void do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb);
private:
  oksvc_procs_t _procs;
};

//-----------------------------------------------------------------------

class okmgr_msg_t : public okmgr_clnt_t {
public:
  okmgr_msg_t (const str &s, oksvc_procs_t p, ok_set_typ_t t, str m);
  void do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb);
private:
  oksvc_procs_t _procs;
  str _msg;
};

//-----------------------------------------------------------------------

class okmgr_diagnostic_t : public okmgr_clnt_t {
public:
  okmgr_diagnostic_t (const str &s, oksvc_proc_t p, 
		      ok_diagnostic_domain_t dd,
		      ok_diagnostic_cmd_t cmd);
  void do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb);
private:
  oksvc_proc_t _proc;
  ok_diagnostic_domain_t _domain;
  ok_diagnostic_cmd_t _cmd;
};

//-----------------------------------------------------------------------

#endif /* _OKD_OKMGR_H */
