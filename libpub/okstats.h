// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Maxwell Krohn (max@okcupid.com)
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

#ifndef _LIBPUB_STATS_H
#define _LIBPUB_STATS_H 1

//
// Work-in-progress for making RPC stats for okws servers talking to
// OKWS DB proxies or RPC servers.  Making histograms for displaying
// distributions of RPC times over the last XX hours and some smart
// historying functions.
//

#include "limits.h"
#include "vec.h"
#include "pubutil.h"

class dataset_t {
public:
  dataset_t () : _min (INT_MAX), _max (INT_MIN) {}
  int min () const { return _min; }
  int max () const { return _max; }
  void add (int p) 
  {
    if (p > _max) { _max = p; }
    if (p < _min) { _min = p; }
    _points.push_back (p);
  }
  size_t size () const { return _points.size (); }
  int operator[] (size_t s) const { return _points[s]; }
private:
  vec<int> _points;
  int _min, _max;
};

class histogram_t {
public:
  /**
   * @param mn minimum value stored in the histogram
   * @param mx maximium value stored
   * @param sz size of each bucket.
   */
  histogram_t (int mn, int mx, int sz)
    : _min (mn), _max (mx), _sz (sz), _underflow (0), _overflow (0),
      _start (okwstime ())
  {
    _buckets.setsize ( (mx - mn + 1) / sz + 1);
    memset (_buckets.base (), 0, sizeof (int) * _buckets.size ());
  }

  void compile (const dataset_t &d);
  bool combine (const histogram_t &h);
  void add_point (int v);
  time_t start () const { return _start; }

  int mode () const { return _mode; }
  u_int operator[] (size_t s) const { return _buckets[s]; }

  enum { UNDERFLOW = -1, OVERFLOW = -2 };

private:
  void do_inc (ssize_t where, u_int howmuch);

  int _min, _max, _sz;
  u_int _underflow, _overflow;
  vec<u_int> _buckets;
  u_int _mode;
  ssize_t _mode_id;
  time_t _start;
};

class histogram_set_t {
public:
  histogram_set_t (const int resolutions[], int n, int x, int s);
  histogram_set_t (const vec<int> &v, int n, int x, int s);
  void add_point (int v);
private:
  void cycle_histograms ();
  ptr<histogram_t> new_hist ();
  void init ();

  ptr<histogram_t> _current;
  ptr<histogram_t> _all;
  vec<vec<ptr<histogram_t> > > _history;
  vec<int> _resolutions;

  int _hmin, _hmax, _hsize;

};

#endif /* STATS_H */
