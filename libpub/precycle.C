// -*-c++-*-

#include "str.h"
#include "okdbg.h"
#include "precycle.h"

//-----------------------------------------------------------------------------

void 
recycler_stats_t::reset_stats() {
  _num_events = 0;
  _num_new = 0;
  _num_delete = 0;
  _num_avg_samples = 0;
  _list_size_sum = 0;
  _max_size_seen = 0;
}

//-----------------------------------------------------------------------------

void 
recycler_stats_t::handle_event(size_t size) {
  ++_num_events;

  // Sample list size
  if (_num_events % (_stats_interval / 100) == 0) {
    ++_num_avg_samples;
    _list_size_sum += size;

    if (size > _max_size_seen) { 
      _max_size_seen = size;
    }
  }

  // Dump stats and then reset counters;
  if (_num_events % _stats_interval == 0) {
    okdbg_warn(CHATTER, 
           strbuf(_name) << ": Recycler stats (for " 
                         << _stats_interval << " events)\n");
    okdbg_warn(CHATTER, 
           strbuf(_name) << ":   calls to new: " << _num_new << "\n");
    okdbg_warn(CHATTER, 
           strbuf(_name) << ":   calls to delete: " << _num_delete << "\n");
    double avg = _list_size_sum / _num_avg_samples;
    okdbg_warn(CHATTER, 
           strbuf(_name) << ":   avg list size: " << avg << "\n");
    okdbg_warn(CHATTER, 
           strbuf(_name) << ":   max list size: " << _max_size_seen << "\n");

    // Resets _num_events to 0
    reset_stats();
  }
}

//-----------------------------------------------------------------------------

void 
recycler_stats_t::alloc_hook(bool can_alloc, size_t size) {
  handle_event(size);
  if (can_alloc) {
    if (_verbose) { 
      okdbg_warn(CHATTER, 
                 strbuf(_name) << ": grabbing obj from trash list"
                               << " at pos " << size - 1 << "\n");
    }
  } else {
    if (_verbose) {
      okdbg_warn(CHATTER, 
                 strbuf(_name) << ": allocating obj (with New)\n");
    }
    _num_new++;
  }
}

//-----------------------------------------------------------------------------

void 
recycler_stats_t::recycle_hook(bool can_recycle, size_t size) {
  handle_event(size);
  if (can_recycle) {
    if (_verbose) {
      okdbg_warn(CHATTER, 
                 strbuf(_name) << ": storing obj on trash list"
                               << " at pos " << size << "\n");
    }
  } else {
    if (_verbose) {
      okdbg_warn(CHATTER, 
                 strbuf(_name) << ": deleting obj\n");
    }
    _num_delete++;
  }
}

//-----------------------------------------------------------------------------

