
#include "lbalance.h"
#include "rxx.h"
#include "parseopt.h"

void
lblnc_tab_t::activate (u_int i)
{
  lblnc_node_t *n = tab[i];
  if (n->active)
    return;
  n->activate ();
  rebuild ();
}

void
lblnc_tab_t::deactivate (u_int i)
{
  lblnc_node_t *n = tab[i];
  if (!n->active)
    return;
  n->deactivate ();
  rebuild ();
}

void
lblnc_node_t::activate ()
{
  active = true;
  get_load_avg ();
  set_timer ();
}

void
lblnc_node_t::set_timer ()
{
  assert (!dcb);
  dcb = delaycb (ok_ldavg_ping_interval, 0, 
		 wrap (this, &lblnc_node_t::timer_event));
}

void
lblnc_node_t::timer_event ()
{
  dcb = NULL;
  get_load_avg ();
  set_timer ();
}

void
lblnc_node_t::deactivate ()
{
  if (dcb) 
    timecb_remove (dcb);
  dcb = NULL;
  active = false;
}

void
lblnc_tab_t::rebuild ()
{
  alive.clear ();
  tab.traverse (wrap (this, &lblnc_tab_t::add_alive_if_active));
}

void
lblnc_tab_t::add_alive_if_active (lblnc_node_t *n)
{
  if (n->active)
    alive.push_back (n);
}

void
lblnc_tab_t::add (lblnc_node_t *n)
{
  assert (!tab[n->id]);
  tab.insert (n);
  if (n->active)
    alive.push_back (n);
}

static rxx host_and_port ("[^:]+(:\\d{1,6})?");

lblnc_t::lblnc_t (pub_t *pub, const str &nm, const rpc_program &rp,
		  int port) :
  prog (rp) 
{
  u_int sz = (*pub)[nm].size ();
  if (sz <= 0) {
    warn << nm << ": no DB array found\n";
    return;
  }
  for (u_int i = 0 ; i < sz ; i++) {
    str s = (*pub)[nm][i];
    if (!host_and_port.match (s) || (!host_and_port[2] && port <= 0)) {
      warn << "invalid DB specified: " << s << "\n";
      continue;
    } 
    int p;
    if (host_and_port[2]) 
      assert (convertint (host_and_port[2], &p));
    else 
      p = port;

    tab.add (New lblnc_node_t (i, rp, host_and_port[1], p, this));
  }
}

lblnc_node_t::lblnc_node_t (u_int i, const rpc_program &rp, 
			    const str &hn, u_int p, lblnc_t *l, 
			    bool a, u_int o) 
  : id (i), hlp (rp, hn, p, o), active (a), dcb (NULL)
{
  hlp.set_status_cb (wrap (l, &lblnc_t::status_change, id));
}


void
lblnc_t::launch (cbv::ptr c)
{
  first_ready_cb = c;
  tab.launch ();
}

static void
hlp_launch (lblnc_node_t *n)
{
  n->hlp.connect ();
}

void
lblnc_tab_t::launch ()
{
  tab.traverse (wrap (hlp_launch));
}

void
lblnc_t::status_change (int i, hlp_status_t st)
{
  if (st == HLP_STATUS_OK) activate (i);
  else deactivate (i);
}

void
lblnc_t::activate (u_int i)
{
  if (first_ready_cb) {
    (*first_ready_cb) ();
    first_ready_cb = NULL;
  }
  tab.activate (i);
}

void
lblnc_node_t::get_load_avg ()
{
  if (!active)
    return;
  hlp.call (LOAD_AVG_RPC, NULL, &load_avg, 
	    wrap (this, &lblnc_node_t::got_load_avg), 
	    ok_ldavg_rpc_timeout);
}

void
lblnc_node_t::got_load_avg (clnt_stat st)
{
  if (!st) 
    load_avg = LOAD_AVG_MAX;
}

void
lblnc_t::call (u_int32_t procno, const void *in, void *out,
	       aclnt_cb cb, time_t duration)
{
  int id = pick_node ();
  if (id < 0) {
    (*cb) (RPC_PROCUNAVAIL);
  } else 
    call (procno, id, in, out, cb, duration);
}
  
int
lblnc_t::pick_node () const
{
  int id;
  int nact = tab.nactive ();
  if (nact == 0) {
    id = -1;
  } else if (nact == 1) {
    id = 0;
  } else {
    int i1 = random () % nact;
    int i2 = random () % nact;
    if (i1 == i2)
      i2 = (i2 + 1) % nact;
    assert (i1 != i2);
    id = (tab[i1]->load_avg < tab[i2]->load_avg) ? i1 : i2;
  }
  return id;
}

void
lblnc_t::call (u_int32_t procno, u_int n, const void *in, void *out,
	       aclnt_cb cb, time_t duration)
{
  tab[n]->hlp.call (procno, in, out, cb, duration);
}

