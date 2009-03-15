
// -*-c++-*-
/* $Id$ */

#ifndef __LIBPUB_OKSYNC_H__
#define __LIBPUB_OKSYNC_H__

#include "tame.h"
#include "tame_lock.h"

namespace oksync {

  //---------------------------------------------------------------------

  class cv_t {
  public:
    cv_t () : _go (false) {}
    void wait (evv_t ev);
    void poke ();
  private:
    bool _go;
    evv_t::ptr _ev;
  };

  //---------------------------------------------------------------------

  class pipeliner_t : public virtual refcount {
  public:
    pipeliner_t ();

    typedef event<ptr<cv_t> >::ref ev_t;

    class stage_t {
    public:
      stage_t () {}
      qhash<size_t, ptr<cv_t> > _waiters;
    };
    
    class handle_t : public virtual refcount {
    public:
      handle_t (ptr<pipeliner_t> p, size_t id) 
	: _parent (p), _id (id), _locked (false), _stage (-1) {}
      ~handle_t ();
      void advance (evv_t evk, CLOSURE);
      void release ();
    protected:
      ptr<pipeliner_t> _parent;
      ptr<cv_t> _curr;
      size_t _id;
      bool _locked;
      ssize_t _stage;
    };

    ptr<handle_t> init ();
    void get_next (size_t stage, size_t id, ev_t ev, CLOSURE);
    
  private:
    size_t _id;
    qhash<size_t, ptr<stage_t> > _stages;
  };
  
  //---------------------------------------------------------------------

};



#endif /* __LIBPUB_OKSYNC_H__ */
