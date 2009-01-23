
#include "okscratch.h"

namespace ok {

  //-----------------------------------------------------------------------

  static scratch_mgr_t g_scratch_mgr;

  //-----------------------------------------------------------------------

  ptr<scratch_handle_t> 
  alloc_scratch (size_t sz) { return g_scratch_mgr.alloc (sz); }

  //-----------------------------------------------------------------------

  ptr<scratch_handle_t> 
  alloc_nonstd_scratch (size_t sz) { return g_scratch_mgr.nonstd_alloc (sz); }

  //-----------------------------------------------------------------------

  scratch_t *
  scrlist_t::alloc ()
  {
    scratch_t *ret = NULL;
    if (_lst.first) {
      ret = _lst.first;
      _lst.remove (ret);
    } else {
      ret = New scratch_t (_size);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  scrlist_t::~scrlist_t ()
  {
    scratch_t *e;
    while ((e = _lst.first)) {
      _lst.remove (e);
      delete e;
    }
  }

  //-----------------------------------------------------------------------

  ptr<scratch_handle_t>
  scratch_mgr_t::alloc (size_t sz)
  {
    ptr<scratch_handle_t> ret;
    scratch_t *s;
    scrlist_t *lst = _h[sz];
    if (!lst) {
      lst = New scrlist_t (sz);
      _h.insert (lst);
    }
    assert (lst);
    s = lst->alloc ();
    ret = New refcounted<scratch_handle_t> (s, this);
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<scratch_handle_t>
  scratch_mgr_t::nonstd_alloc (size_t sz)
  {
    scratch_mgr_t *mgr = NULL;
    scratch_t *s = New scratch_t (sz);
    return New refcounted<scratch_handle_t> (s, mgr);
  }

  //-----------------------------------------------------------------------

  void
  scrlist_t::reclaim (scratch_t *s)
  {
    assert (s->len() == _size);
    _lst.insert_head (s);
  }

  //-----------------------------------------------------------------------

  void
  scratch_mgr_t::reclaim (scratch_t *s)
  {
    size_t l = s->len ();
    scrlist_t *lst = _h[l];
    if (lst) {
      lst->reclaim (s);
    } else {
      delete s;
    }
  }

  //-----------------------------------------------------------------------

  scratch_handle_t::~scratch_handle_t ()
  {
    if (_mgr) {
      _mgr->reclaim (_scratch);
    } else {
      delete _scratch;
    }
  }

  //-----------------------------------------------------------------------

  ptr<scratch_handle_t> 
  scratch_handle_t::grow (size_t sz)
  {
    assert (sz > len ());
    ptr<scratch_handle_t> n;
    n = scratch_mgr_t::nonstd_alloc (sz);
    memcpy (n->buf (), buf (), len ());
    return n;
  }

  //-----------------------------------------------------------------------

};
