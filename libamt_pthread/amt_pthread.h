// -*-c++-*-
/* $Id: amt_pthread.h 4522 2009-06-08 19:31:51Z max $ */

#pragma once

#include "amt.h"
#include <pthread.h>

class mpt_dispatch_t : public mtdispatch_t // Posix Threads
{
public:
  mpt_dispatch_t (newthrcb_t c, u_int n, u_int m, ssrv_t *s);
  ~mpt_dispatch_t () { warn << "in ~mpt_dispatch_t\n"; delete [] pts; } 
  void launch (int i, int fdout);
  void giant_lock ();
  void giant_unlock ();
protected:
  pthread_t *pts;
  pthread_mutex_t _giant_lock;
};

