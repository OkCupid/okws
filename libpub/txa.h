// -*-c++-*-

/*
 * txa.h
 *
 *    Thin XDR Authentication layer for rpcc compiler.
 *    Runtime system and policy engine.
 *
 * $Id$
 *
 */

#ifndef _TXA_H_INCLUDED
#define _TXA_H_INCLUDED

#include "ihash.h"
#include "str.h"
#include "async.h"

struct txa_tok_t {
  txa_tok_t (const str &k, bool p = true) : tok (k), positive (p) {}
  str tok;
  bool positive;
  ihash_entry<txa_tok_t> lnk;
};

class txa_node_t;
typedef ihash<str, txa_tok_t, &txa_tok_t::tok, &txa_tok_t::lnk> txa_toktab_t;

class txa_prog_t;
class txa_node_t {
  friend class txa_prog_t;
protected:
  txa_node_t (int i = 0) : id (i), toktab (NULL), tab (NULL), toks (false) {}

public:
  inline int access (const str &s) const 
  { 
    if (!toktab) return 0;
    txa_tok_t *t = (*toktab)[s];
    if (!t) return 0;
    else if (t->positive) return 1;
    else return -1;
  }
  bool empty () const { return (!toks); }

  inline txa_node_t & operator[] (int i)
  {
    if (!tab) 
      tab = New ihash<int, txa_node_t, &txa_node_t::id, &txa_node_t::lnk> ();
    txa_node_t *p = (*tab)[i];
    if (p) 
      return *p;
    else {
      toks = true;
      p = New txa_node_t (i);
      tab->insert (p);
      return *p;
    }
  }

  inline void insert (const str &k, bool p = true) 
  { 
    if (!toktab)
      toktab = New txa_toktab_t ();
    toktab->insert (New txa_tok_t (k, p)); 
    toks = true;
  }

  int id;
  ihash_entry<txa_node_t> lnk;
  txa_toktab_t *toktab;
protected:
  ihash<int, txa_node_t, &txa_node_t::id, &txa_node_t::lnk> *tab;
private:
  bool toks;
};

#define RPC_LEVELS 2

class txa_prog_t : public txa_node_t
{
public:
  txa_prog_t () : txa_node_t (), login_rpc (0) {}

  inline u_int32_t get_login_rpc () const { return login_rpc; }

  inline bool 
  authorized (const vec<str> &toks, int proc_id) const
  {
    // if no auth tokens, assume that everything is open
    if (empty ()) return true;

    const txa_node_t *stack[RPC_LEVELS];
    memset (stack, 0, sizeof (txa_node_t *) * RPC_LEVELS);
    stack[0] = this;
    stack[1] = (*tab)[proc_id];

    int rc = 0;
    for (u_int i = RPC_LEVELS - 1; i >= 0; i--) {
      if (stack[i]) {
	bool reject = false;
	for (u_int j = 0; j < toks.size (); j++) {
	  rc = stack[i]->access (toks[j]);
	  if (rc == 1) return true;
	  if (rc == -1) reject = true;
	}
	if (reject) return false;
      }
    }

    // if there are *some* auth tokens, but no persmission explicitly
    // granted with these tokens and with this call, then the default
    // policy is "no"
    return false;
  }
protected:
  void set_login_rpc (u_int32_t i) { login_rpc = i; }
private:
  u_int32_t login_rpc;
};

#endif /* _TXA_H_INCLUDED */
