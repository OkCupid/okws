
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxmlxlatemgr.h"
#include "okconst.h"
#include <limits.h>


namespace okxml {

  enum { PORT_MAX = 0x10000 };

  str
  to_netloc_hash (const str &h, int p)
  {
    return strbuf ("%s:%d", h.cstr (), p);
  }

  conn_t::conn_t (const str &h, int p)
    : _hostname (h),
      _port (p),
      _hashkey (to_netloc_hash (h, p)),
      _created (sfs_get_timenow ()),
      _self (mkref (this))
  {}

  void
  conn_t::touch ()
  {
    _accessed = sfs_get_timenow ();
  }

  void
  conn_t::release ()
  {
    _self = NULL;
  }

  tamed void 
  conn_t::connect (cbb cb)
  {
    tvars {
      int fd;
      bool ret (false);
    }
    twait { tcpconnect (_hostname, _port, mkevent (fd)); }
    if (fd >= 0) {
      _x = axprt_stream::alloc (fd, ok_axprt_ps);
      ret = true;
    }
    (*cb) (ret);
  }

  bool
  conn_t::connected () const
  {
    return (_x && !_x->ateof ());
  }

  tamed void
  connmgr_t::getcon (const str &h, int p, xcb_t cb)
  {
    tvars {
      conn_t *c;
      ptr<conn_t> ch;
      bool ok;
      bool do_release (false);
      ptr<axprt> x;
    }
    c = _tab[to_netloc_hash (h, p)];
    if (c) {
      ch = mkref (c);
      _q.remove (c);
      _q.insert_tail (c);
      c->touch ();
    } else {
      ch = New refcounted<conn_t> (h, p);
      c = ch;
      _tab.insert (c);
      _q.insert_tail (c);
    }

    if (!c->connected ()) {
      twait { c->_lock.acquire (tame::lock_t::EXCLUSIVE, mkevent ()); }
      twait { c->connect (mkevent (ok)); }
      do_release = true;
    } else {
      ok = true;
    }

    if (ok) {
      x = c->x ();
    }

    (*cb)(x);

    if (do_release)
      c->_lock.release ();
  }

  void
  xlate_mgr_t::add_program (const xml_rpc_program *p)
  {
    _programs.insert (p->xdr_prog->name, p);
  }

  void
  xlate_mgr_t::add_const (const xml_rpc_const_t *c)
  {
    _constants.insert (c->name, c->val);
  }

  void
  xlate_mgr_t::add_files (const xml_rpc_file *const *list)
  {
    for (const xml_rpc_file *const *p = list; *p; p++) {
      add_file (**p);
    }
  }

  void
  xlate_mgr_t::add_file (const xml_rpc_file &file)
  {
    _files.insert (file.filename, &file);
    for (const xml_rpc_program **p = file.programs; *p; p++) {
      add_program (*p);
    }
    for (const xml_rpc_const_t *c = file.constants; c->name; c++) {
      add_const (c);
    }
  }

  class collector_t : public xml_pound_def_collector_t {
  public:

    struct xml_pair_t {
      xml_pair_t (const str &n) : _name (n) {}
      virtual ~xml_pair_t () {}

      void output (xml_obj_ref_t o) 
      {
	o[0] = _name;
	output_v (o[1]);
      }

    protected:
      const str _name;
      virtual void output_v (xml_obj_ref_t o) = 0;
    };
    
    template<class T>
    struct tpair_t : public xml_pair_t {
      tpair_t (const str &s, T i) : xml_pair_t (s), _value (i) {}
      void output_v (xml_obj_ref_t o) { o = _value; }
    private:
      const T _value;
    };

    collector_t () {}

    ~collector_t() 
    {
      while (_v.size ()) 
	delete _v.pop_back ();
    }

    void collect (const str &s, str v)
    { _v.push_back (New tpair_t<str> (s, v)); }
    void collect (const str &s, int i) 
    { _v.push_back (New tpair_t<int> (s, i));}
    void collect (const str &s, const char *v) 
    { _v.push_back (New tpair_t<const char *> (s, v));}

    void output (xml_obj_ref_t o, size_t offset)
    {
      for (size_t k = 0; k < _v.size (); k++) {
	_v[k]->output (o[offset+k]);
      }
    }

  private:
    vec<xml_pair_t *> _v;
  };

  void
  xlate_mgr_t::get_constants (xml_req_t in, xml_resp_cb_t cb)
  {
    xml_resp_t resp;
    size_t k = 0;
    collector_t collector;
    for (size_t i = 0; i < in.size (); i++) {
      for (size_t j = 0; j < in[i].size (); j++) {
	str f = in[i][j];
	const xml_rpc_file **x;
	if (f && (x = _files[f])) {
	  for (const xml_rpc_const_t *c = (*x)->constants; c->name; c++) {
	    resp[0][k][0] = c->name;
	    resp[0][k][1] = c->val;
	    k++;
	  }
	  if ( (*x)->pound_def_collector )
	    (*x)->pound_def_collector (&collector);
	}
      }
    }
    collector.output (resp[0], k);
    cb->trigger (resp);
  }

  tamed void
  xlate_mgr_t::xlate (xml_obj_const_t input, xml_resp_cb_t cb)
  {
    tvars {
      str hostname;
      int port;
      str program;
      int procno;
      xml_obj_const_t arg;
      ptr<axprt> x;
      clnt_stat stat;
      const xml_rpc_program **progp;
      ptr<aclnt> c;
      void *xdr_arg (NULL), *xdr_res (NULL);
      const rpcgen_table *tblent_xdr;
      const xml_rpcgen_table *tblent_xml;
      xml_resp_t resp;
      XML_reader_t xml_arg;
      XML_creator_t xml_res;
      str errm;
    }

    hostname = input("hostname");
    port = input ("port");
    program = input ("program");
    procno = input ("procno");
    arg = input ("arg");
    xml_arg.setroot (arg);

    if (!hostname || port <= 0 || port >= PORT_MAX) {
      stat = RPC_UNKNOWNHOST;
    } else {
      twait { _cmgr.getcon (hostname, port, mkevent (x)); }
      if (!x) {
	stat = RPC_UNKNOWNADDR;
      } else if (!program || !(progp = _programs[program])) {
	stat = RPC_PROGUNAVAIL;
      } else if (procno < 0 || procno > int ((*progp)->xdr_prog->nproc)) {
	stat = RPC_PROCUNAVAIL;
      } else {
	tblent_xml = (*progp)->xml_tbl + procno;
	tblent_xdr = (*progp)->xdr_prog->tbl + procno;

	xdr_arg = (*tblent_xdr->alloc_arg) ();
	if (!tblent_xml->xml_arg_proc (&xml_arg, xdr_arg)) {
	  stat = RPC_CANTDECODEARGS;
	  errm = xml_arg.error_msg (clnt_sperrno (stat));
	} else {
	  c = aclnt::alloc (x, *((*progp)->xdr_prog));
	  xdr_res = (*tblent_xdr->alloc_res) ();
	  twait { c->call (procno, xdr_arg, xdr_res, mkevent (stat)); }
	  if (!stat) {
	    xml_res.setroot (resp[0]);
	    if (!tblent_xml->xml_res_proc (&xml_res, xdr_res)) {
	      stat = RPC_CANTDECODERES;
	      errm = xml_res.error_msg (clnt_sperrno (stat));
	    }
	  }
	}
      }
    }
    if (xdr_arg) {
      xdr_free (tblent_xdr->xdr_arg, xdr_arg);
      xfree (xdr_arg);
    }
    if (xdr_res) {
      xdr_free (tblent_xdr->xdr_res, xdr_res);
      xfree (xdr_res);
    }

    if (stat) {
      if (!errm)
	errm = clnt_sperrno (stat);
      resp = xml_fault_obj_t (int (stat), errm);
    }
      
    (*cb) (resp);
  }

}

