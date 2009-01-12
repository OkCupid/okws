// -*-c++-*-
/* $Id$ */

#ifndef __LIBAOK_OK3_H__
#define __LIBAOK_OK3_H__

#include "ok.h"

/*
 * okclnt3_t: like okclnt_t and okclnt2_t, a class that corresponds to
 * an incoming HTTP client request.  okclnt3_t is difference since it
 * supports HTTP/1.1 pipelining, and therefore can accept multiple
 * requests per one connection.
 */
class okclnt3_t : public okclnt_interface_t {
public:

  //------------------------------------------------------------------------

  class cv_t {
  public:
    cv_t () : _go (false) {}
    void wait (evv_t ev);
    void poke ();
  private:
    bool _go;
    evv_t::ptr _ev;
  };

  //------------------------------------------------------------------------


  class req_t : public http_parser_cgi_t, public virtual refcount {
  public:
    req_t (ptr<ahttpcon> x, u_int to);
    ~req_t ();

    typedef event<int, bool>::ref parse_ev_t;

    void parse (parse_ev_t, CLOSURE); 
    http_inhdr_t *hdr_p () { return http_parser_cgi_t::hdr_p (); }
    const http_inhdr_t &hdr_cr () const { return http_parser_cgi_t::hdr_cr (); }

    void set_union_cgi_mode (bool b)
    { http_parser_cgi_t::set_union_mode (b); }
  };

  //------------------------------------------------------------------------

  class resp_t : public virtual refcount {
  public:
    resp_t (okclnt3_t *o, ptr<req_t> q);
    ~resp_t ();

    //-----------------------------------------------------------------------

    void error (int status) { reply (status, NULL, NULL); }
    void redirect (int status, str u) { reply (status, NULL, u); }
    void reply (int status, ptr<compressible_t> body, str redir_url);

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

    //-----------------------------------------------------------------------

    void mark_defunct () { _ok_clnt = NULL; }
    bool is_ready () const { return _replied; }
    void send (evb_t ev, CLOSURE);

    //-----------------------------------------------------------------------

    ptr<req_t> req () { return _req; }
    int status () const { return _status; }

    //-----------------------------------------------------------------------

  protected:
    okclnt3_t *_ok_clnt;
    vec<ptr<cookie_t> > _outcookies;
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

    ptr<vec<http_hdr_field_t> > _hdr_fields;

    ptr<req_t> _req;
  };

  //------------------------------------------------------------------------

  okclnt3_t (ptr<ahttpcon> xx, oksrvc_t *o, u_int to = 0);
  ~okclnt3_t ();

  //------------------------------------------------------------------------

  virtual void process (ptr<req_t> req, ptr<resp_t> resp, evv_t ev) = 0;

  //------------------------------------------------------------------------

  virtual bool ssl_only () const { return false; } 
  virtual str  ssl_redirect_str () const { return NULL; }
  bool is_ssl () const { return _demux_data && _demux_data->ssl (); }
  str ssl_cipher () const;

  //------------------------------------------------------------------------

  void set_localizer (ptr<const pub_localizer_t> l);
  ptr<pub2::ok_iface_t> pub2 ();
  ptr<pub2::ok_iface_t> pub2_local ();

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
  bool check_ssl ();
  void redirect (int status, const str &u);
  void error (int status);

  //-----------------------------------------------------------------------

  void poke ();
  void output_loop (int time_budget, evv_t ev, CLOSURE);
  void finish_output (evv_t ev);
  void await_poke (evv_t ev);

  //-----------------------------------------------------------------------

  ptr<ahttpcon> _x;
  u_int _timeout;

  ptr<demux_data_t> _demux_data;
  ptr<pub2::locale_specific_publisher_t> _p2_locale;
  vec<ptr<resp_t> > _resps;

  bool _union_cgi_mode;

  //-----------------------------------------------------------------------

  cv_t _output_cv;

  //-----------------------------------------------------------------------

};



#endif /* __LIBAOK_OK3_H__ */
