
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
    pipeliner_t (size_t nstages);
    
    class handle_t : public virtual refcount {
    public:
      handle_t (ptr<pipeliner_t> p) 
	: _parent (p), _stage (-1), _locked (false) {}
      ~handle_t ();
      void advance (evv_t evk, CLOSURE);
    protected:
      ptr<pipeliner_t> _parent;
      ssize_t _stage;
      bool _locked;
    };

    ptr<handle_t> init ();
    void release (size_t s);
    void acquire (size_t s, evv_t ev);
    
  private:
    vec<tame::lock_t> _stages;
  };
  
  //---------------------------------------------------------------------

};



#endif /* __LIBPUB_OKSYNC_H__ */
