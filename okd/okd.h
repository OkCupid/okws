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
#include "axprtfd.h"
#include "fd_prot.h"

#define OK_LQ_SIZE_D    100
#define OK_LQ_SIZE_LL   5
#define OK_LQ_SIZE_UL   1000


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
  okch_t (okd_t *o, const str &e);
  ~okch_t ();
  void launch ();
  void clone (ref<ahttpcon_clone> xc);
  void shutdown (oksig_t sig, cbv cb);

  void got_new_ctlx_fd (int fd, int p);
  void got_new_x_fd (int fd, int p);

  void dispatch (ptr<bool> destroyed, svccb *b);
  void repub (ptr<ok_repub_t> rpb);
  void repub_cb (ptr<ok_repub_t> rpb, clnt_stat err);

  void fdcon_eof (ptr<bool> destroyed);
  void kill ();
  void chld_eof (ptr<bool> dfp);
  
  okd_t *myokd;
  int pid;
  const str servpath;       // GET <servpath> HTTP/1.1 (starts with '/')
  ihash_entry<okch_t> lnk;
protected:
  void start_chld ();
private:
  void shutdown_cb1 (cbv cb);

  ptr<ahttpcon> x;

  vec<ptr<ahttpcon_clone> > conqueue;

  okc_state_t state;
  ptr<bool> destroyed;
};

class okd_t : public ok_httpsrv_t 
{
public:
  okd_t (const str &cf, int logfd_in, int okldfd_in, const str &cdd) : 
    ok_httpsrv_t (NULL, logfd_in),
    okd_usr (ok_okd_uname), okd_grp (ok_okd_gname),
    pubd (NULL), 
    configfile (cf),
    okldfd (okldfd_in),
    pprox (New pub_proxy_t ()),
    sdflag (false), sd2 (false), dcb (NULL), listenfd (-1), sdattempt (0),
    cntr (0),
    coredumpdir (cdd)
  {}

  ~okd_t ();

  void abort ();

  void insert (okch_t *c) { servtab.insert (c); }
  void remove (okch_t *c) { servtab.remove (c); }

  void got_alias (vec<str> s, str loc, bool *errp);
  void got_pubd_unix (vec<str> s, str loc, bool *errp);
  void got_pubd_inet (vec<str> s, str loc, bool *errp);
  void got_pubd_exec (vec<str> s, str loc, bool *errp);

  void gotfd (int fd, ptr<okws_fd_t> desc);

  void pubconf (svccb *sbp);
  void getfile (svccb *sbp);
  void lookup (svccb *sbp);
  void req_errdocs (svccb *sbp);

  void pubconfed (ptr<xpub_getfile_res_t> r, clnt_stat err);
  void lookedup (str fn, ptr<xpub_lookup_res_t> r, clnt_stat err);
  void gotfile (phashp_t hsh, ptr<xpub_getfile_res_t> r, clnt_stat err);

  // Well, no one ever said event-driven programming was pretty
  void launch ();
  void launch2 ();
  void launch3 ();

  void launch_logd ();
  void launch_pubd ();
  void launch_logd_cb (bool rc);
  void launch_pubd_cb (bool err);

  void parseconfig ();
  void sclone (ref<ahttpcon_clone> x, str s, int status);
  void newserv (int fd);
  void shutdown (int sig);

  ihash<const str, okch_t, &okch_t::servpath, &okch_t::lnk> servtab;
  qhash<str, str> aliases;

  ok_usr_t okd_usr; 
  ok_grp_t okd_grp;

  helper_t *pubd;

  void open_mgr_socket ();

  void repub (const xpub_fnset_t &x, okrescb cb);
  void repub_cb1 (ptr<ok_repub_t> rpb, clnt_stat err);
  void repub_cb2 (ptr<int> i, okrescb cb, ptr<ok_res_t> res);

  void relaunch (const ok_progs_t &x, okrescb cb);

  void turnlog (okrescb cb);

  void strip_privileges ();

  void kill_srvcs_cb ();
  bool in_shutdown () const { return sdflag; }
  void set_signals ();

private:
  void newmgrsrv (ptr<axprt_stream> x);
  void check_runas ();

  // shutdown functions
  void kill_srvcs (oksig_t sig);
  void stop_listening ();
  void shutdown_retry ();
  void shutdown2 ();
  void shutdown3 ();
  void shutdown_cb1 ();

  void got_chld_fd (int fd, ptr<okws_fd_t> desc);

  str configfile;
  int okldfd;
  
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
  bool jailed;
  int sdattempt;
  int cntr;

  ptr<fdsource_t<okws_fd_t> > okldx;
  str coredumpdir;
};

class okd_mgrsrv_t 
{
public:
  okd_mgrsrv_t (ptr<axprt_stream> xx, okd_t *o);
  void dispatch (svccb *b);
private:
  void repub (svccb *b);
  void relaunch (svccb *b);
  void turnlog (svccb *b);
  ptr<asrv> srv;
  ptr<axprt_stream> x;
  okd_t *myokd;
};

class common_404_t {
public:
  common_404_t ();
  bool operator[] (const str &s) { return tab[s]; }
  bhash<str> tab;
};

#endif /* _OKD_OKD_H */
