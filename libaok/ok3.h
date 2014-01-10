// -*-c++-*-
/* $Id$ */

#ifndef __LIBAOK_OK3_H__
#define __LIBAOK_OK3_H__

#include "ok.h"
#include "oksync.h"

/*
 * okclnt3_t: like okclnt_t and okclnt2_t, a class that corresponds to
 * an incoming HTTP client request.  okclnt3_t is difference since it
 * supports HTTP/1.1 pipelining, and therefore can accept multiple
 * requests per one connection.
 */
class okclnt3_t : public okclnt_interface_t {
public:

  //------------------------------------------------------------------------

  class req_t : public http_parser_cgi_t, public virtual refcount {
  public:
    req_t (okclnt3_t *o, ptr<ahttpcon> x, abuf_t *b, u_int rn, 
	   htpv_t prev_vers, u_int to);
    ~req_t ();

    typedef event<int, bool>::ref parse_ev_t;

    void parse (parse_ev_t, CLOSURE); 
    http_inhdr_t *hdr_p () { return http_parser_cgi_t::hdr_p (); }
    const http_inhdr_t &hdr_cr () const { return http_parser_cgi_t::hdr_cr (); }

    void set_union_cgi_mode (bool b)
    { http_parser_cgi_t::set_union_mode (b); }

    htpv_t http_vers () const { return hdr.get_vers (); }
    okclnt3_t *ok_clnt () { return _ok_clnt; }
    const okclnt3_t *ok_clnt () const { return _ok_clnt; }

    virtual bool want_raw_body() override { return _ok_clnt->want_raw_body(); }
  private:
    okclnt3_t *_ok_clnt;
  };

  //------------------------------------------------------------------------

  class resp_t : public outcookie_holder_t, 
		 public virtual refcount {
  public:
    resp_t (okclnt3_t *o, ptr<req_t> q);
    ~resp_t ();

    //-----------------------------------------------------------------------

    typedef event<ptr<resp_t> >::ref ev_t;

    //-----------------------------------------------------------------------

    void error (int status, str s = NULL) { reply (status, NULL, NULL, s); }
    void redirect (int status, str u) { reply (status, NULL, u, NULL); }
    void ok (int status, ptr<compressible_t> body) { reply (status, body); }

    void reply (int status, ptr<compressible_t> body, 
		str url = NULL, str es = NULL);

    void set_release_ev (evv_t::ptr ev) { _release_ev = ev; }

    //-----------------------------------------------------------------------

    ptr<cookie_t> add_cookie (const str &h = NULL, const str &p = "/");
    void set_uid (u_int64_t i) { _uid = i; _uid_set = true; }

    //-----------------------------------------------------------------------

    void set_content_type (const str &s) { _content_type = s; }
    void set_cache_control (const str &s) { _cache_control = s; }
    void set_expires (const str &s) { _expires = s; }
    void set_content_disposition (const str &s) { _cont_disp = s; }
    void disable_gzip () { _rsp_gzip = false; }
    void set_custom_log2 (const str &s) { _custom_log2 = s; }
    void set_hdr_field (const str &k, const str &v);

    //-----------------------------------------------------------------------

    void set_attributes (http_resp_attributes_t *hra);
    void set_connection_attributes (http_resp_attributes_t *hra);
    void set_error_attributes (http_resp_attributes_t *hra);
    void fixup_log (ptr<http_response_base_t> rsp);

    //-----------------------------------------------------------------------

    void mark_defunct () { _ok_clnt = NULL; }
    bool is_ready () const { return _replied; }
    void send (evb_t ev, time_t time_budget = 0, int *nsp = NULL, CLOSURE);
    void cancel (int status, evv_t ev, CLOSURE);
    bool keep_serving() { return _serving; }

    //-----------------------------------------------------------------------

    void set_log_fixup_cb (cbv::ptr cb) { _log_fixup_cb = cb; }
    void set_error_ok(bool ok) { _error_ok = ok; }
    void set_add_connection(bool add) { _add_connection = add; }
    void set_keep_serving(bool ks) { _serving = ks; }

    //-----------------------------------------------------------------------

    ptr<req_t> req () { return _req; }
    int status () const { return _status; }
    gzip_mode_t do_gzip () const;
    ptr<ahttpcon> con ();
    oksrvc_t *svc ();

    //-----------------------------------------------------------------------

  protected:

    //-----------------------------------------------------------------------

    void do_release_ev ();

    //-----------------------------------------------------------------------

    okclnt3_t *_ok_clnt;
    ptr<http_response_t> _http_resp;
    u_int64_t _uid;
    bool _uid_set;

    str _content_type, _cache_control, _expires, _cont_disp;
    str _custom_log2;
    bool _rsp_gzip;

    bool _sent, _replied;

    int _status;
    ptr<compressible_t> _body;
    str _redir_url;
    str _error_str;

    ptr<vec<http_hdr_field_t> > _hdr_fields;

    ptr<req_t> _req;
    evv_t::ptr _release_ev;
    cbv::ptr _log_fixup_cb;

    bool _error_ok;
    bool _serving;
    bool _add_connection;
  };

  //------------------------------------------------------------------------

  class rrpair_t : public virtual okrrp_interface_t  {
  public:
    rrpair_t (ptr<req_t> rq, ptr<resp_t> resp)
      : _req (rq), _resp (resp) {}
    
    void set_custom_log2 (const str &log) { _resp->set_custom_log2 (log); }
    void disable_gzip () { _resp->disable_gzip (); }
    void set_expires (const str &s)  { _resp->set_expires (s); }

    void set_hdr_field (const str &k, const str &v) 
    { _resp->set_hdr_field (k, v); }

    void set_cache_control (const str &s) { _resp->set_cache_control (s); }
    void set_content_type (const str &s) { _resp->set_content_type (s); }

    // access input parameters
    const http_inhdr_t &hdr_cr () const { return _req->hdr_cr (); }
    virtual http_inhdr_t *hdr_p () { return _req->hdr_p (); }

    // output paths...
    void okreply (ptr<compressible_t> c, evv_t::ptr ev);
    void redirect (const str &s, int status, evv_t::ptr ev);
    void error (int n, const str &s, evv_t::ptr ev);

    void set_log_fixup_cb (cbv::ptr ev);

    ptr<demux_data_t> demux_data ();
    ptr<const demux_data_t> demux_data () const;

  private:

    void output_T (compressible_t *c, evv_t::ptr ev, CLOSURE);
    void redirect_T (const str &s, int status, evv_t::ptr ev, CLOSURE);
    void error_T (int n, const str &s, bool dm, evv_t::ptr ev, CLOSURE);

    ptr<req_t> _req;
    ptr<resp_t> _resp;
  };

  //------------------------------------------------------------------------

  okclnt3_t (ptr<ahttpcon> xx, oksrvc_t *o, u_int to = 0);
  ~okclnt3_t ();

  //------------------------------------------------------------------------

  virtual void process (ptr<req_t> req, ptr<resp_t> resp, evv_t ev) = 0;

  // convert to a standard request that looks like HTTP/1.0
  ptr<rrpair_t> convert (ptr<req_t> req, ptr<resp_t> resp);

  //------------------------------------------------------------------------

  // Override and return true if you want to handle parsing the HTTP body
  // yourself (e.g. as JSON).
  virtual bool want_raw_body() { return false; }

  virtual bool ssl_only () const { return false; } 
  virtual str  ssl_redirect_str () const { return NULL; }
  bool is_ssl () const { return _demux_data && _demux_data->ssl (); }
  str ssl_cipher () const;

  //------------------------------------------------------------------------

  ptr<demux_data_t> demux_data () { return _demux_data; }
  ptr<const demux_data_t> demux_data () const { return _demux_data; }

  //------------------------------------------------------------------------

  void set_localizer (ptr<const pub3::localizer_t> l);
  ptr<pub3::ok_iface_t> pub3 ();
  ptr<pub3::ok_iface_t> pub3_local ();
  ptr<ahttpcon> con () { return _x; }

  //------------------------------------------------------------------------

  void set_union_cgi_mode (bool b) { _union_cgi_mode = b; }
  void set_demux_data (ptr<demux_data_t> d)  { _demux_data = d; }
  virtual void serve () { serve_T (); }

  //------------------------------------------------------------------------

protected:

  //------------------------------------------------------------------------

  void serve_T (CLOSURE);
  virtual void finish_serve () { delete this; }

  //-----------------------------------------------------------------------
  
  ptr<resp_t> alloc_resp (ptr<req_t> r = NULL);
  ptr<req_t> alloc_req (u_int rn, htpv_t prev);
  bool check_ssl ();
  void redirect (int status, const str &u);
  void error (int status, ptr<req_t> r = NULL);

  //-----------------------------------------------------------------------

  void poke ();
  void output_loop (int time_budget, evv_t ev, CLOSURE);
  void wait_for_ready_output (okclnt3_t::resp_t::ev_t ev, CLOSURE);
  void finish_output (evv_t ev);
  void await_poke (evv_t ev);

  //-----------------------------------------------------------------------

  ptr<ahttpcon> _x;
  abuf_t *_abuf;
  u_int _timeout;

  ptr<demux_data_t> _demux_data;
  ptr<pub3::ok_iface_t> _p3_locale;
  vec<ptr<resp_t> > _resps;

  bool _union_cgi_mode;
  bool _serving;

  //-----------------------------------------------------------------------

  oksync::cv_t _output_cv;

  //-----------------------------------------------------------------------

};

//-----------------------------------------------------------------------

// Constants for dealining with keepalives
namespace okclnt3 {
  extern int n_keepalives;
  extern int keepalive_timeout;
};

//-----------------------------------------------------------------------


#endif /* __LIBAOK_OK3_H__ */
