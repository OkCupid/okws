// -*-c++-*-

#ifndef _OKD_OKD_H
#define _OKD_OKD_H

#include <sys/time.h>
#include "async.h"
#include "arpc.h"
#include "ahttp.h"
#include "cgi.h"
#include "resp.h"
#include "ok.h"
#include "pslave.h"
#include "okerr.h"
#include "svq.h"
#include "okconst.h"

#define OK_LQ_SIZE_D    100
#define OK_LQ_SIZE_LL   5
#define OK_LQ_SIZE_UL   1000

typedef enum { OKC_STATE_NONE = 0,
	       OKC_STATE_LAUNCH = 1,
	       OKC_STATE_SERVE = 2,
	       OKC_STATE_HOSED = 3,
	       OKC_STATE_DELAY = 4 } okc_state_t;

typedef enum { OKD_JAILED_NEVER = 0,
	       OKD_JAILED_DEPENDS = 1,
	       OKD_JAILED_ALWAYS = 2 } okd_jailtyp_t;

struct ok_repub_t {
  ok_repub_t (const xpub_fnset_t &f, okrescb c)
    : fnset (f), cb (c), res (New refcounted<ok_res_t> ()) {}
  ~ok_repub_t () { (*cb) (res); } 
  void set_new_fnset ();
  const xpub_fnset_t &fnset;
  xpub_fnset_t new_fnset;
  const okrescb cb;
  ptr<ok_res_t> res;
  xpub_result_t xpr;
  ok_xstatus_t xst;
};

class okd_t;
class okch_t : public ok_con_t {  // OK Child Handle
public:
  okch_t (const str &e, const str &s, okd_t *o, const str &cfl);
  ~okch_t ();
  void launch ();
  void clone (ref<ahttpcon_clone> xc);
  void fdcon_eof (ptr<bool> destroyed);
  void dispatch (ptr<bool> destroyed, svccb *b);
  void kill ();
  void resurrect ();

  void repub (ptr<ok_repub_t> rpb);
  void repub_cb (ptr<ok_repub_t> rpb, clnt_stat err);

  void relaunch (ptr<ok_res_t> res);
  bool can_exec ();
  void shutdown (oksig_t sig, cbv cb);
  
  int pid;
  const str rexecpath;      // execpath relative to jaildir (starts with '/')
  const str servpath;       // GET <servpath> HTTP/1.1 (starts with '/')
  ihash_entry<okch_t> lnk;
protected:
private:
  void shutdown_cb1 (cbv cb);
  void launch_cb (ptr<bool> destroyed, int fd);

  ptr<ahttpcon> x;

  vec<ptr<ahttpcon_clone> > conqueue;
  vec<struct timeval *> timevals;

  okc_state_t state;
  okd_t *myokd;
  str cfgfile_loc;
  ptr<bool> destroyed;
  timecb_t *rcb;
};

class okd_t : public ok_base_t
{
public:
  okd_t () : ok_base_t (), listenaddr (INADDR_ANY),
	     okd_usr (ok_uname), svc_usr (ok_svc_uname),
	     okd_grp (ok_gname), svc_grp (ok_svc_gname),
	     pubd (NULL), uid (getuid ()),
	     pprox (New pub_proxy_t ()), bdlnch (false),
	     sdflag (false), sd2 (false), dcb (NULL), listenfd (-1),
	     lexc (NULL), jailed (false) {}

  ~okd_t ();

  void insert (okch_t *c) { servtab.insert (c); }
  void remove (okch_t *c) { servtab.remove (c); }

  void got_bindaddr (vec<str> s, str loc, bool *errp);
  void got_alias (vec<str> s, str loc, bool *errp);
  void got_cgiserver (vec<str> s, str loc, bool *errp);
  void got_pubd_unix (vec<str> s, str loc, bool *errp);
  void got_pubd_inet (vec<str> s, str loc, bool *errp);
  void got_pubd_exec (vec<str> s, str loc, bool *errp);

  void got_logd_exec (vec<str> s, str log, bool *errp);

  void pubconf (svccb *sbp);
  void getfile (svccb *sbp);
  void lookup (svccb *sbp);
  void req_errdocs (svccb *sbp);

  void pubconfed (ptr<xpub_getfile_res_t> r, clnt_stat err);
  void lookedup (str fn, ptr<xpub_lookup_res_t> r, clnt_stat err);
  void gotfile (phashp_t hsh, ptr<xpub_getfile_res_t> r, clnt_stat err);

  void launch (const str &cf);
  void launch2 ();
  void launchservices ();
  void launchservices2 ();
  void checkservice (okch_t *s);
  void launch_pubd ();
  void launch_logd ();
  void launch_pubd_cb (bool err);
  void launch_logd_cb (bool err);

  void parseconfig (const str &cf);
  void sclone (ref<ahttpcon_clone> x, str s, int status);
  void newserv (int fd);
  str  make_execpath (const str &rexe, bool chrt);
  void shutdown (int sig);

  ihash<const str, okch_t, &okch_t::servpath, &okch_t::lnk> servtab;
  qhash<str, str> aliases;

  u_int32_t listenaddr;
  str listenaddr_str;

  ok_usr_t okd_usr, svc_usr; // personalities for running OKD services
  ok_grp_t okd_grp, svc_grp; // groups for running OKD services

  logd_parms_t logd_parms;

  helper_t *pubd;

  int uid;      // what the server is launch as

  cgi_t env;    // execution environment
  str pdjdir;   // pub daemon jail dir

  void open_mgr_socket ();

  void repub (const xpub_fnset_t &x, okrescb cb);
  void repub_cb1 (ptr<ok_repub_t> rpb, clnt_stat err);
  void repub_cb2 (ptr<int> i, okrescb cb, ptr<ok_res_t> res);

  void relaunch (const ok_progs_t &x, okrescb cb);

  void chroot ();
  void set_svc_ids ();
  void set_okd_ids ();
  void badlaunch () { bdlnch = true; }

  void kill_srvcs_cb ();
  bool in_shutdown () const { return sdflag; }

private:
  void encode_env ();
  void newmgrsrv (ptr<axprt_stream> x);
  void get_runas (const str &cf);

  // shutdown functions
  void kill_srvcs (oksig_t sig);
  void stop_listening ();
  void shutdown_retry ();
  void shutdown2 ();
  void shutdown3 ();
  void shutdown_cb1 ();

  pub_proxy_t *pprox;

  svq_t   <ptr<xpub_getfile_res_t> >            cfq;  // conf fetch Q
  svqtab_t<pfnm_t, ptr<xpub_lookup_res_t> >     luq;  // lookup Q
  svqtab_t<phashp_t, ptr<xpub_getfile_res_t> >  gfq;  // Getfile Q

  u_int launches;
  bool bdlnch;
  u_int chkcnt;
  u_int sdcbcnt;
  bool sdflag, sd2;
  timecb_t *dcb;
  int listenfd;
  helper_exec_t *lexc;
  bool jailed;
};

class okd_mgrsrv_t 
{
public:
  okd_mgrsrv_t (ptr<axprt_stream> xx, okd_t *o);
  void dispatch (svccb *b);
private:
  void repub (svccb *b);
  void relaunch (svccb *b);
  ptr<asrv> srv;
  ptr<axprt_stream> x;
  okd_t *myokd;
};

#endif /* _OKD_OKD_H */
