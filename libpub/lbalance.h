
// -*-c++-*-
/* $Id$ */

#ifndef _LIBPUB_LBALANCE_H
#define _LIBPUB_LBALANCE_H 1

#include "pslave.h"
#include "pub.h"

#define LOAD_AVG_MAX      UINT_MAX
#define LOAD_AVG_RPC      100

class lblnc_t;
class lblnc_node_t {
public:
  lblnc_node_t (u_int i, const rpc_program &rp, const str &hn, u_int p, 
		lblnc_t *l, bool a = false, u_int o = 0) ;
  void get_load_avg ();
  void got_load_avg (clnt_stat st);
  void activate ();
  void deactivate ();

  u_int id;
  ihash_entry<lblnc_node_t> hlnk;
  helper_inet_t hlp;
  bool active;
  u_int load_avg;

private:
  void set_timer ();
  void timer_event ();
  timecb_t *dcb;
};

class lblnc_tab_t {
public:
  lblnc_tab_t () {}
  ~lblnc_tab_t () {}
  lblnc_node_t *operator[] (u_int i) const { return tab[i]; }
  int nactive () const { return alive.size (); }
  void add (lblnc_node_t *n);
  void activate (u_int i);
  void deactivate (u_int i);
  void add_alive_if_active (lblnc_node_t *n);
  void launch ();
private:
  void rebuild ();
  vec<lblnc_node_t *> alive;
  ihash <u_int, lblnc_node_t, &lblnc_node_t::id, &lblnc_node_t::hlnk> tab;
};

class lblnc_t : public helper_base_t {
public:
  lblnc_t (pub_t *pub, const str &i, const rpc_program &rp, int port = -1) ;
  virtual ~lblnc_t () {}
  void launch (cbv::ptr c);
  void connect (cbb::ptr c);
  str getname () const;

  // ask the load balancer to find the best-suited remote server
  void call (u_int32_t procno, const void *in, void *out, aclnt_cb cb,
	     time_t duration = 0);

  // tell the load balancer which remote server to use
  void call (u_int32_t procno, u_int n, const void *in, void *out,
	    aclnt_cb cb, time_t duration = 0);

  void status_change (int i, hlp_status_t st);

protected:
  virtual int pick_node () const;

private:
  void activate (u_int i);
  void deactivate (u_int i) { tab.deactivate (i); }
  void connect_cb_success ();
  void connect_cb_fail ();
  void call_connect_cb (bool arg);

  str name;
  const rpc_program prog;
  lblnc_tab_t tab;

  cbv::ptr first_ready_cb;
  cbb::ptr connect_cb;
  timecb_t *dcb;
};

#endif /* _LIBPUB_LBALANCE_H */
