
#include "lbalance.h"


void
lblnc_tab_t::activate (u_int i)
{
  lblnc_node_t *n = tab[i];
  if (n->active)
    return;
  n->active = true;
  rebuild ();
  return true;
}

void
lblnc_tab_t::deactivate (u_int i)
{
  lblnc_node_t *n = tab[i];
  if (!n->active)
    return;
  n->active = false;
  rebuild ();
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
