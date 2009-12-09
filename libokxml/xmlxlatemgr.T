
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
  xlate_mgr_t::add_type (const xml_typeinfo_t *t)
  {
    _types.insert (t->name, t);
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
    for (const xml_typeinfo_t **t = file.types; *t; t++) {
      add_type (*t);
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

  xlate_mgr_t::xlate_mgr_t()
  {
    for (const xml_typeinfo_t **t = xml_typeinfo_base_types; *t; t++) {
      add_type (*t);
    }
  }

  void
  xlate_mgr_t::fill_struct_info(xml_obj_ref_t x, 
				const xml_struct_entry_t *entries, int type, 
				vec<str> &out, bhash<str> &seen)
  {
    size_t i = 0;
    bool done = false;
    while (!done) {
      bool hasAny = false;
      if (entries[i].type) {
        str type = entries[i].type->name;
        x[i]("type") = type; hasAny = true;
        if (!seen[type] && _types[type]) {
          seen.insert(type);
          out.push_back(type);
        }
      }
      if (entries[i].name) { 
	x[i]("name") = str(entries[i].name); 
	hasAny = true; 
      }
      if (entries[i].swval) { 
	x[i]("swval") = str(entries[i].swval); 
	hasAny = true; 
      }
      if (hasAny) {
        x[i]("qual") = entries[i].qual;
        x[i]("bound") = entries[i].bound;
      }
      done = !hasAny || (type == xml_typeinfo_t::TYPEDEF);
      i++;
    }
  }

  void
  xlate_mgr_t::fill_type_info(xml_obj_ref_t x, const xml_typeinfo_t *type, 
			      vec<str> &out, bhash<str> &seen)
  {
    x("type") = type->name;
    switch (type->category) {
    case xml_typeinfo_t::PRIMITIVE: x("category") = str("PRIMITIVE"); break;
    case xml_typeinfo_t::ENUM: x("category") = str("ENUM"); break;
    case xml_typeinfo_t::STRUCT:
      x("category") = str("STRUCT");
      fill_struct_info(x("members"), type->contents, 
		       xml_typeinfo_t::STRUCT, out, seen);
      break;
    case xml_typeinfo_t::UNION:
      x("category") = str("UNION");
      fill_struct_info(x("members"), type->contents, 
		       xml_typeinfo_t::UNION, out, seen);
      break;
    case xml_typeinfo_t::TYPEDEF:
      x("category") = str("TYPEDEF");
      fill_struct_info(x("alias"), type->contents, 
		       xml_typeinfo_t::TYPEDEF, out, seen);
    };
  }

  void
  xlate_mgr_t::get_types(xml_req_t in, xml_resp_ev_t cb)
  {
    xml_resp_t resp;
    bhash<str> seen;
    vec<str> out;
    for (size_t i = 0; i < in.size (); i++) {
      for (size_t j = 0; j < in[i].size (); j++) {
	str n = in[i][j];
	const xml_typeinfo_t **t;
	if (!n || !(t = _types[n])) continue;

        if (seen[(*t)->name]) continue;
        out.push_back((*t)->name);
        seen.insert((*t)->name);
      }
    }
    for (size_t i = 0; i < out.size(); i++) {
      fill_type_info(resp[0](out[i]), *_types[out[i]], out, seen);
    }

    cb->trigger(resp);
  }

  void
  xlate_mgr_t::get_prog_info (xml_req_t in, xml_resp_ev_t cb)
  {
    xml_resp_t resp;
    for (size_t i = 0; i < in.size (); i++) {
      for (size_t j = 0; j < in[i].size (); j++) {
        str p = in[i][j];
        const xml_rpc_program **x;
        if (!p || !(x = _programs[p])) continue;
        for (size_t k = 0; k < (*x)->nproc; k++) {
          if (! (*x)->xml_tbl[k].name) { 
            resp(p)[k]("name") = "null";
            continue;
          }
          resp[0](p)[k]("name") = (*x)->xml_tbl[k].name;
          resp[0](p)[k]("arg") = (*x)->xml_tbl[k].xml_arg_type->name;
          resp[0](p)[k]("res")= (*x)->xml_tbl[k].xml_res_type->name;
        }
      }
    }
    cb->trigger (resp);
  }

  void
  xlate_mgr_t::get_constants (xml_req_t in, xml_resp_ev_t cb)
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

  static const char *
  my_clnt_sperrno (clnt_stat stat)
  {
    const char *ret;
    switch (stat) {
    case RPC_UNKNOWNADDR:
      ret = "Cannot open TCP connection to given host/port";
      break;
    default:
      ret = clnt_sperrno (stat);
      break;
    }
    return ret;
  }

  static str
  make_rpc_errmsg (const str &hostname, int port, clnt_stat stat)
  {
    str who;
    if (hostname) {
      strbuf b; 
      b << hostname << ":" << port;
      who = b;
    } else {
      who  = "(N/A)";
    }
    
    strbuf b; 
    str m = my_clnt_sperrno (stat);
    b << "To " << who << ": " << m;
    return b;
  }

  xlate_retry_mgr_t::xlate_retry_mgr_t  ()
    : xlate_mgr_t ()
  {
    timespec ts[] = { { 0, 500000000 },
		      { 1, 0 },
		      { 3, 0 },
		      { 7, 0 },
		      { 0, 0 } };

    for (timespec *t = ts; t->tv_sec || t->tv_nsec; t++) {
      _delays.push_back (*t);
    }
  }

  static bool retry_it (clnt_stat s) 
  { return (s == RPC_CANTSEND || s == RPC_CANTRECV || s == RPC_UNKNOWNADDR); }

  tamed void
  xlate_retry_mgr_t::do_rpc_T (str hostname, int port, 
			       const rpc_program &prog,
			       int procno, const void *arg, void *res,
			       aclnt_cb cb)
  {
    tvars {
      ptr<axprt> x;
      clnt_stat stat;
      ptr<aclnt> c;
      ssize_t i (-1);
      
    }

    do {

      if (i >= 0) {

	{
	  str m = make_rpc_errmsg (hostname, port, stat);
	  warn << "** attempt " << i << ": " << m << "\n";
	}

	twait { 
	  delaycb (_delays[i].tv_sec, _delays[i].tv_nsec, mkevent ()); 
	}
      }
    
      twait { _cmgr.getcon (hostname, port, mkevent (x)); }
      if (!x) {
	stat = RPC_UNKNOWNADDR;
      } else {
	c = aclnt::alloc (x, prog);
	twait { c->call (procno, arg, res, mkevent (stat)); }
      }

    } while (retry_it (stat) && ++i < ssize_t (_delays.size ()));

    cb->trigger (stat);
  }


  tamed void
  xlate_mgr_t::do_rpc_T (str hostname, int port, 
			 const rpc_program &prog,
			 int procno, const void *arg, void *res,
			 aclnt_cb cb)
  {
    tvars {
      ptr<axprt> x;
      clnt_stat stat;
      ptr<aclnt> c;
    }
    twait { _cmgr.getcon (hostname, port, mkevent (x)); }
    if (!x) {
      stat = RPC_UNKNOWNADDR;
    } else {
      c = aclnt::alloc (x, prog);
      twait { c->call (procno, arg, res, mkevent (stat)); }
    }
    cb->trigger (stat);
  }


  tamed void
  xlate_mgr_t::xlate (xml_obj_const_t input, xml_resp_ev_t cb)
  {
    tvars {
      str hostname;
      int port;
      str program;
      int procno;
      xml_obj_const_t arg;
      clnt_stat stat;
      const xml_rpc_program **progp;
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
	errm = xml_arg.error_msg (my_clnt_sperrno (stat));
      } else {
	xdr_res = (*tblent_xdr->alloc_res) ();
	twait { 
	  do_rpc (hostname, port, *((*progp)->xdr_prog),
		  procno, xdr_arg, xdr_res, mkevent (stat)); 
	}
	if (!stat) {
	  xml_res.setroot (resp[0]);
	  if (!tblent_xml->xml_res_proc (&xml_res, xdr_res)) {
	    stat = RPC_CANTDECODERES;
	    errm = xml_res.error_msg (my_clnt_sperrno (stat));
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

      str errmsg_rpc = make_rpc_errmsg (hostname, port, stat);

      warn << errmsg_rpc << "\n";
      if (!errm) {
	errm = errmsg_rpc;
      }

      resp = xml_fault_obj_t (int (stat), errm);
    }
      
    cb->trigger (resp);
  }

}

