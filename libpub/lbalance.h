
// -*-c++-*-
/* $Id$ */

#ifndef _LIBPUB_LBALANCE_H
#define _LIBPUB_LBALANCE_H 1

#include "pslave.h"
#include "pub.h"

class lblnc_node_t {
public:
  lblnc_node_t (u_int i, const rpc_program &rp, const str &hn, u_int p, 
		bool a, u_int o = 0) 
    : id (i), hlp (rp, hn, p, o), active (a) {}
  u_int id;
  ihash_entry<lblnc_node_t> hlnk;
  helper_inet_t hlp;
  bool active;
};

class lblnc_tab_t {
public:
  lblnc_tab_t () {}
  ~lblnc_tab_t () {}
  lblnc_node_t *operator[] (u_int i) const { return tab[i]; }
  void add (lblnc_node_t *n)
  void activate (u_int i);
  void deactivate (u_int i);
  void add_alive_if_active (lblnc_node_t *n);
private:
  void rebuild ();
  vec<lblnc_node_t *> alive;
  ihash <u_int, lblnc_node_t, &lblnc_node_t::id, &lblnc_node_t::hlnk> tab;
};

class lblnc_t {
public:
  lblnc_t (pub_t *pub, const str &i, const rpc_program &rp) ;
  void launch (cbb bool);

  //
  // ask the load balancer to find the best-suited remote server
  //
  int call (u_int32_t procno, const void *in, const void *out, aclnt_cb cb,
	    time_t duration = 0);

  //
  // tell the load balancer which remote server to use
  //
  int call (u_int32_t procno, u_int n, const void *in, const void *out,
	    aclnt_cb cb, time_t duration = 0);

private:
  void activate (u_int i);
  void deactivate (u_int i);

  const rpc_progname prog;
};

#endif /* _LIBPUB_LBALANCE_H */
