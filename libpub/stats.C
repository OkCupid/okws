
#include "okstats.h"


void
histogram_t::compile (const dataset_t &d)
{
  for (size_t s = 0; s < d.size (); s++) {
    add_point (d[s]);
  }
}

void
histogram_t::add_point (int v)
{
  ssize_t id;
  if (v < _min) { 
    id = UNDERFLOW;
  } else if (v > _max) { 
    id = OVERFLOW;
  } else {
    id = (v - _min) / _sz;
  }
  do_inc (id, 1);
}

void
histogram_t::do_inc (ssize_t where, u_int howmuch)
{
  u_int nv;
  if (where == UNDERFLOW) {
    nv =  (_underflow  += howmuch) ;
  } else if (where == OVERFLOW) {
    nv = ( _overflow += howmuch );
  } else {
    nv = ( _buckets[where] += howmuch );
  }
  if (nv > _mode) {
    _mode = nv;
    _mode_id = where;
  }
}

bool
histogram_t::combine (const histogram_t &h)
{
  if (_min != h._min || _max != h._max || _sz != h._sz)
    return false;
  
  do_inc (UNDERFLOW, h._underflow);
  do_inc (OVERFLOW, h._overflow);
  for (size_t s = 0; s < _buckets.size (); s++) 
    do_inc (s, h[s]);

  return true;
}

static int qcmp (const void *a, const void *b)
{
  if(a > b)
      return 1;
  else if (a < b)
      return -1;
  return 0;
}

histogram_set_t::histogram_set_t (const int v[], int n, int x, int s)
  : _hmin (n), _hmax (x), _hsize (s)
{
  for (const int *p = v; p; p++) {
    _resolutions.push_back (*p);
  }
  init ();
}

histogram_set_t::histogram_set_t (const vec<int> &v, int n, int x, int s)
  : _hmin (n), _hmax (x), _hsize (s)
{
  for (size_t s = 0; s < v.size (); s++) {
    _resolutions.push_back (s);
  }
}

void
histogram_set_t::init ()
{
  assert (_resolutions.size () > 0);
  _current = new_hist ();
  qsort (_resolutions.base (), _resolutions.size (), sizeof (int), qcmp);
  _history.setsize (_resolutions.size ());
}

ptr<histogram_t>
histogram_set_t::new_hist ()
{
  return New refcounted<histogram_t> (_hmin, _hmax, _hsize);
}

void
histogram_set_t::add_point (int v)
{
  cycle_histograms ();
  _current->add_point (v);
}

void
histogram_set_t::cycle_histograms ()
{
  if (okwstime () - _current->start () > _resolutions[0]) {
    _history[0].push_back (_current);
    for (size_t s = 0 ; s < _history.size () - 1; s++) {
      ptr<histogram_t> sum, h;
      while (_history[s].size () &&
	     okwstime () - _history[s][0]->start () > _resolutions[s+1]) {
	h = _history[s].pop_front ();
	if (!sum) sum = h;
	else { sum->combine (*h); }
      }
      if (sum) {
	ptr<histogram_t> nxt = _history[s+1].front ();
	if (nxt) nxt->combine (*sum);
	else _history[s+1].push_back (sum);
      }
    }
    assert (_history.back ().size () <= 1);
    _current = new_hist ();
  }
}
