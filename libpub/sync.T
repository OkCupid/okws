// -*-c++-*-

#include "oksync.h"

namespace oksync {
  
  //--------------------------------------------------------------------
  
  void
  cv_t::wait (evv_t ev)
  {
    if (_go) {
      assert (!_ev);
      _go = false;
      ev->trigger ();
    } else {
      _ev = ev;
    }
  }
  
  //-----------------------------------------------------------------------
  
  void
  cv_t::poke ()
  {
    if (_ev) {
      assert (!_go);
      evv_t tmp (_ev);
      _ev = NULL;
      tmp->trigger ();
    } else {
      _go = true;
    }
  }
  
  //--------------------------------------------------------------------

  pipeliner_t::pipeliner_t (size_t nstages)
  {
    _stages.setsize (nstages);
  }

  //--------------------------------------------------------------------

  ptr<pipeliner_t::handle_t>
  pipeliner_t::init ()
  {
    return New refcounted<handle_t> (mkref (this));
  }

  //--------------------------------------------------------------------

  pipeliner_t::handle_t::~handle_t ()
  {
    if (_locked) { _parent->release (_stage); }
  }

  //--------------------------------------------------------------------

  tamed void
  pipeliner_t::handle_t::advance (evv_t ev)
  {
    tvars {
      holdvar ptr<pipeliner_t::handle_t> hold (mkref (_self));
      size_t nxtstage;
      size_t oldstage;
    }

    oldstage = _stage;
    nxtstage = oldstage + 1;

    twait { _parent->acquire (nxtstage, mkevent ()); }
    _stage = nxtstage;
    _locked = true;

    ev->trigger ();
    
    if (oldstage >= 0) {
      _parent->release (oldstage);
    }
  }

  //--------------------------------------------------------------------

  void
  pipeliner_t::acquire (size_t s, evv_t ev)
  {
   _stages[s].acquire (tame::lock_t::EXCLUSIVE, ev);
  }

  //--------------------------------------------------------------------

  void
  pipeliner_t::release (size_t s)
  {
    _stages[s].release ();
  }

  //--------------------------------------------------------------------

};
