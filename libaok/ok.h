
// -*-c++-*-
/* $Id$ */

#ifndef _LIBAOK_OK_H
#define _LIBAOK_OK_H

#include "arpc.h"
#include "ahttp.h"
#include "cgi.h"
#include "resp.h"
#include "pub.h"
#include "okprot.h"
#include "inhdr.h"
#include "pslave.h"
#include "pubutil.h"
#include "oklog.h"
#include "zstr.h"
#include "ahparse.h"


#define SVCWARN(x) \
  warn << "pid " << pid << ": " << x << "\n";

struct errdoc_t {
  errdoc_t (int n, const str &f) : status (n), fn (f) {}
  int status;
  str fn;
  ihash_entry<errdoc_t> lnk;
};

class ok_con_t {
public:
  ok_con_t () {}
protected:
  void ctlcon (callback<void, svccb *>::ref cb);
  void ctlclose ();

  ptr<axprt_unix> ctlx;
  ptr<aclnt> clnt;
  ptr<asrv> srv;
};

class ok_base_t : public ok_con_t { 
public:
  ok_base_t (const str &h = NULL) : 
    jaildir (ok_jaildir), version (ok_version), hostname (h),
    listenport (ok_dport), topdir (ok_topdir), okdname (ok_dname),
    jailed (false), logd (NULL), logfd (-1) {}
  virtual ~ok_base_t () { errdocs.deleteall (); }
  inline bool add_errdoc (int n, const str &f);
  void add_errdocs (const xpub_errdoc_set_t &eds);
  virtual void add_pubfile (const str &s) {}

  virtual void error (ref<ahttpcon> x, int n, str s = NULL, cbv::ptr c = NULL,
		      http_inhdr_t *h = NULL) const;
  virtual str servinfo () const;
  virtual ptr<http_response_t> geterr (int n, str s, htpv_t v) const;
  virtual str jail2real (const str &fn) const;
  virtual void log (ref<ahttpcon> x, http_inhdr_t *req, http_response_t *res,
		    const str &s = NULL)
    const { if (logd) logd->log (x, req, res, s); }
  log_t *get_logd () { return logd; }

  str jaildir;
  str version;
  str hostname;
  u_int16_t listenport;
  str topdir;
  str okdname;
  bool jailed;
  pub_t pub;

protected:
  void error2 (ref<ahttpcon> x, int n, str s, cbv::ptr c, http_inhdr_t *h) 
    const;
  void error_cb1 (ptr<http_parser_raw_t> p, int n, str s, cbv::ptr c,
		  int s2) const;
  void error_cb2 (ptr<ahttpcon> x, ptr<http_response_t> e, cbv::ptr c) 
    const { if (c) (*c) (); }

  ihash<int, errdoc_t, &errdoc_t::status, &errdoc_t::lnk> errdocs;
  mutable str si;
  log_t *logd;
  int logfd;
  str logfmt;
};

#define OKCLNT_BUFLEN 0x10400
#define OKCLNT_BUFLEN2 0x4000

class oksrvc_t;
class okclnt_t 
  : public http_parser_cgi_t { // One for each external HTTP client
public:
  okclnt_t (ptr<ahttpcon> xx, oksrvc_t *o, u_int to = 0) : 
    http_parser_cgi_t (xx, to), x (xx), oksrvc (o)
  {}

  virtual ~okclnt_t ();
  virtual void serve ();
  virtual void error (int n, const str &s = NULL);
  virtual void process () = 0;
  virtual void parse ();
  virtual void output (zbuf &b);
  virtual void redirect (const str &s);
  virtual void send (ptr<http_response_t> rsp);
  virtual cookie_t *add_cookie (const str &h = NULL, const str &p = "/");

  list_entry<okclnt_t> lnk;
protected:
  void http_parse_cb (int status);
  void delcb () { delete this; }
		
  cbv::ptr cb;
  ref<ahttpcon> x;
  oksrvc_t *oksrvc;

  //strbuf out;
  zbuf out;
  ptr<http_response_t> rsp;
  vec<cookie_t *> outcookies;
};

typedef callback<okclnt_t *, ptr<ahttpcon>, oksrvc_t *>::ref nclntcb_t;

class dbcon_t : public helper_inet_t {
public:
  dbcon_t (const rpc_program &g, const str &h, u_int p)
    : helper_inet_t (g, h, p, 0) {}
  str getname () const { return strbuf ("database: ") << 
			   helper_inet_t::getname () ;}
};

class oksrvc_t : public ok_base_t { // OK Service
public:
  oksrvc_t (int argc, char *argv[]) 
    : nclients (0), sdflag (false), pid (getpid ())
  { 
    init (argc, argv);
  }

  virtual void launch ();
  virtual okclnt_t *make_newclnt (ptr<ahttpcon> lx) = 0;
  virtual void init_publist () {}
  virtual u_int get_andmask () const { return 0xffffffff; }
  virtual u_int get_ormask () const { return 0; }
  virtual void custom_init (cbv cb) { (*cb) (); }

  ~oksrvc_t () {}
  void init (int argc, char *argv[]);
  void shutdown ();
  void connect ();
  void ctldispatch (svccb *c);
  void remove (okclnt_t *c);
  void add (okclnt_t *c);
  void end_program ();

  void add_pubfiles (const char *arr[], u_int sz, bool conf = false);
  void add_pubfiles (const char *arr[], bool conf = false);
  void add_pubfile (const str &s, bool conf = false);

  str cfg (const str &n) const { return rpcli->cfg (n); }
  template<class C> bool cfg (const str &n, C *v) const 
  { return rpcli->cfg (n, v); }
  template<typename T> parr_err_t cfg (const str &n, u_int i, T *p) const;
  void pubfiles (cbb cb);
  dbcon_t *add_db (const str &host, u_int port, const rpc_program &p);

  pval_w_t operator[] (const str &s) const { return (*rpcli)[s]; }

protected:

  void pubbed (cbb cb, ptr<pub_res_t> res);

  void launch_log_cb (bool rc);
  void launch2 (bool rc);
  void launch_pub_cb0 (ptr<pub_res_t> r);
  void launch_pub_cb1 (ptr<xpub_errdoc_set_t> xd, clnt_stat err);
  void launch_pub_cb2 (bool rc);
  void launch3 ();
  void launch4 ();
  void launch5 (clnt_stat err);
  void launch_dbs (cbb b);
  void launch_dbcb (cbb b, u_int i, bool r);

  void newclnt (ptr<ahttpcon> lx);
  void update (svccb *sbp);
  void kill (svccb *v);
  void update_cb (svccb *sbp, ptr<pub_res_t> pr);
  void ready_call (bool rc);

  str name;
  list<okclnt_t, &okclnt_t::lnk> clients;
  ptr<ahttpcon_listen> x;

  u_int nclients;
  bool sdflag;
  pub_rclient_t *rpcli;

  vec<dbcon_t *> dbs;
  bool dbstatus;
  u_int dbl;
  u_int lnum;
  int pid;
};

class oksrvcw_t : public oksrvc_t { // OK Service Wrapped
public:
  oksrvcw_t (int argc, char *argv[], nclntcb_t c) : 
    oksrvc_t (argc, argv), nccb (c) {}
  okclnt_t *make_newclnt (ptr<ahttpcon> lx) { return (*nccb) (lx, this); }
private:
  nclntcb_t nccb;
}; 

struct ok_idpair_t {
  ok_idpair_t (const str &n) : name (n), id (-1) {}
  virtual ~ok_idpair_t () {}
  virtual bool resolve () const = 0;

  operator bool () const
  { 
    bool ret = name && resolve (); 
    if (name && !ret)
      warn << "Could not find " << typ () << " \"" << name << "\"\n";
    return ret;
  }
  virtual str typ () const = 0;

  str name;
  mutable int id;
};

struct ok_usr_t : public ok_idpair_t {
  ok_usr_t (const str &n) : ok_idpair_t (n) {}
  bool resolve () const { return ((id = uname2uid (name)) >= 0); }
  str typ () const { return "user"; }
};

struct ok_grp_t : public ok_idpair_t {
  ok_grp_t (const str &n) : ok_idpair_t (n) {}
  bool resolve () const { return ((id = gname2gid (name)) >= 0); }
  str typ () const { return "group"; }
};

template<typename T> parr_err_t 
oksrvc_t::cfg (const str &n, u_int i, T *p) const
{
  pval_t *v;
  const parr_ival_t *arr;
  if (!rpcli->cfg (n, &v))
    return PARR_NOT_FOUND;
  if (!(arr = v->to_int_arr ()))
    return PARR_BAD_TYPE;
  return arr->val (i, p);
}
  

#endif /* _LIBAOK_OKBASE_H */
