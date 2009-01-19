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

  pipeliner_t::pipeliner_t () : _id (0) {}

  //--------------------------------------------------------------------

  ptr<pipeliner_t::handle_t>
  pipeliner_t::init ()
  {
    return New refcounted<handle_t> (mkref (this), _id++);
  }

  //--------------------------------------------------------------------

  void
  pipeliner_t::handle_t::release ()
  {
    if (_curr) (_curr->poke ());
  }

  //--------------------------------------------------------------------

  pipeliner_t::handle_t::~handle_t ()
  {
    release ();
  }

  //--------------------------------------------------------------------

  tamed void
  pipeliner_t::handle_t::advance (evv_t ev)
  {
    tvars {
      holdvar ptr<pipeliner_t::handle_t> hold (mkref (_self));
    }

    release ();
    _stage++;
    twait { _parent->get_next (_stage, _id, mkevent (_curr)); }
    ev->trigger ();
  }

  //--------------------------------------------------------------------

  tamed void
  pipeliner_t::get_next (size_t stage, size_t id, pipeliner_t::ev_t ev)
  {
    tvars {
      ptr<pipeliner_t::stage_t> *sp, s;
      ptr<cv_t> c, *cp, p, *pp;
      ssize_t pid (id - 1);
    }

    sp = _stages[stage];

    if (!sp) {
      s = New refcounted<stage_t> ();
      _stages.insert (stage, s);
    } else {
      s = *sp;
    }

    if (pid >= 0) {
      pp = s->_waiters[pid];
      if (!pp) {
	p = New refcounted<cv_t> ();
	s->_waiters.insert (pid, p);
      } else {
	p = *pp;
      }
      twait { p->wait (mkevent ()); }
      s->_waiters.remove (pid);
    }

    cp = s->_waiters[id];
    if (cp) {
      c = *cp;
    } else {
      c = New refcounted<cv_t> ();
      s->_waiters.insert (id, c);
    }

    ev->trigger (c);
  }

  //--------------------------------------------------------------------

};
