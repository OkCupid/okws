
// -*-c++-*-
/* $Id$ */

#ifndef _LIBAHTTP_APARSE_H
#define _LIBAHTTP_APARSE_H

#include "abuf.h"

//
// Asynchronous Parser Skeleton Class
//
class async_parser_t {
public:
  async_parser_t () 
    : abuf (NULL), nwabuf (false), parsing (false), dataready (false) {}
  async_parser_t (abuf_src_t *a)
    : abuf (New abuf_t (a)), nwabuf (true), parsing (false), 
      dataready (false) {}
  async_parser_t (abuf_t *a) 
    : abuf (a), nwabuf (false), parsing (false), dataready (false) {}
  virtual ~async_parser_t () { if (nwabuf) delete (abuf); }
  
  void parse (cbi::ptr c = NULL);
  void can_read_cb ();
  void cancel ();
  void finish_parse (int r = 0);
  void resume ();

  void reset () 
  { 
    if (abuf) abuf->reset ();
    parsing = dataready = false; 
  }
  
  // where the real guts of the async parsing is done -- not here
  virtual void parse_guts () = 0;
  abuf_t *get_abuf () { return abuf; }

protected:
  abuf_t *abuf;
private:
  cbi::ptr pcb;
  bool nwabuf;
  bool parsing;
  bool dataready;
};

class async_dumper_t : public async_parser_t {
public:
  async_dumper_t (abuf_t *a) : async_parser_t (a), buf (NULL) {}
  ~async_dumper_t () { if (buf) delete buf; }

  void dump (size_t len, cbs c);
protected:
  void parse_guts ();
  void parse_done_cb (int dummy) { (*dump_cb) (*buf); dump_cb = NULL; }

  mstr *buf;
  char *bp, *endp;
  cbs::ptr dump_cb;
};

#endif
