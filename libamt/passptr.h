// -*-c++-*-

#ifndef __LIBAMT__PASSPTR_H__
#define __LIBAMT__PASSPTR_H__

#include "refcnt.h"

//
// Passptr
//
//    Here's the idea: one thread holds onto an object, has a number
//    or references to the object (perhaps by passing it from one function
//    to another) but eventually wants to pass all references and control
//    of that object to a different thread.  This class allows that.
//
//    The general setup is:
//
//       passptr<foo> x (New refcounted<foo> ());
//       passptr<foo> y = x;
//  
//    Now y and x both point to the same **slot** which in turn both
//    points to the same object.  Now, a handoff to a different thread:
//
//       passptr<foo> z;
//       y.pass_to (z);
//
//    As a result, z now has control of the object, and the slot that
//    x and y share now is pointing to an empty object.
//
//       assert (!x);
//       assert (!y);
//
//    Should both work!
//
//  

//-----------------------------------------------------------------------

//
// a passptr_slot is the slot that stores a reference to an object.
// passptr's point to a passptr_slot.  the level of indirection allows
// the behavior explained above.
//
template<class T>
class passptr_slot {
public:
  template<class U> passptr_slot(ptr<U> p) : _obj (p) {}
  passptr_slot () {}
  
  void clear () { _obj = NULL; }
  ptr<T> obj () { return _obj; }
  ptr<const T> obj () const { return _obj; }
  
  template<class U> void set (ptr<U> p) { _obj = p; }

private:
  ptr<T> _obj;
};

//-----------------------------------------------------------------------

//
// the smart pointer operations that need to be allowed for a given
// passptr.  note that we had to make an exception for void, since
// doing "void *p = blah; *p = 4;" is bad and won't compile!
//
template<class T>
class pp_refops {
public:

#define DEREF(cnst, star) \
    cnst T *o = obj ();	  \
    assert (o);		  \
    return star o;

    T &operator* () { DEREF(,*); }
    const T &operator* () const { DEREF(const,*); }
    T *operator-> () { DEREF(,); }
    const T *operator-> () const { DEREF(const,); }
    
#undef DEREF
  
  // filled in by passptr below!
  virtual const T *obj () const = 0;
  virtual T *obj() = 0;

};

//-----------------------------------------------------------------------

// special case -- no ref operations for a void...
template<> class pp_refops<void> {};

//-----------------------------------------------------------------------

//
// passptr<T> -- the main class. implements the smart pointer interface
// and also a call to pass_to and/or take_from, as described above.
//
template<class T>
class passptr : public pp_refops<T> {
public:

  passptr (ptr<T> p) : _slot (New refcounted<passptr_slot<T> > (p)) {}
  passptr () : _slot (New refcounted<passptr_slot<T> > ()) {}

  virtual ~passptr () {}

  template<class U> void pass_to (passptr<U> p)
  {
    ptr<T> o = obj_p ();
    if (o) { p.set (o); }
    else { p.clear (); }
    clear ();
  }

  template<class U> void take_from (passptr<U> p)
  {
    ptr<U> o = p.obj_p ();
    if (o) { set (o); }
    else { clear (); }
    p.clear ();
  }

  template<class U> void set (ptr<U> u) { _slot->set (u); }
  passptr<T> &operator= (ptr<T> x) { set (x); return *this; }

  operator bool() const { return obj(); }
  void clear () { _slot->clear (); }
  ptr<T> obj_p () const { return _slot->obj (); }
  const T *obj() const { return _slot->obj (); }
  T *obj () { return _slot->obj (); }
  const T *get() const { return obj (); }
  T *get () { return obj (); }

private:
  ref<passptr_slot<T> > _slot;

};

//-----------------------------------------------------------------------

#endif /* __LIBAMT__PASSPTR_H__ */
