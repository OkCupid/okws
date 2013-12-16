// -*-c++-*-
/* $Id$ */
/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include "ahparse.h"

//-----------------------------------------------------------------------

http_parser_base_t::~http_parser_base_t ()
{
  if (tocb) {
    timecb_remove (tocb);
    tocb = NULL;
  }
 
  if (_del_abuf && _abuf) {
    delete _abuf;
    _abuf = NULL;
  }

  *destroyed = true;
}

//-----------------------------------------------------------------------

void
http_parser_base_t::parse (cbi c)
{
  cb = c;
  tocb = delaycb (timeout, 0, wrap (this, &http_parser_base_t::clnt_timeout));
  _parsing_header = true;
  hdr_p ()->parse (wrap (this, &http_parser_base_t::parse_cb1));
}

//-----------------------------------------------------------------------

void
http_parser_base_t::parse_cb1 (int status)
{
  if (_abuf) { _header_sz = _abuf->get_ccnt (); }
  _parsing_header = false;
  if (status != HTTP_OK) {
    finish (status);
    return;
  }
  v_parse_cb1 (status);
}

//-----------------------------------------------------------------------

void
http_parser_base_t::finish (int status)
{
  if (tocb) {
    timecb_remove (tocb);
    tocb = NULL;
  }

  // If we don't stop the abuf, we might be fooled into parsing
  // again on an EOF.
  stop_abuf ();

  // XXX -- we (stupidly) have a cyclic data structure here; that is,
  // we might have a refcounted this wrapped in the callback given.
  // thus, if we haven't be destroyed, we'll need to set the CB equal 
  // to NULL.  in some cases, so doing will cause the object to be 
  // destroyed.
  //
  // this seems to be the safest way to go about this. we'll have cleared
  // cb before we make the (potentially destructive) call to tcbi.
  // when tcbi goes out of scope as the function returns, this might
  // cause this object to be deleted. 

  cbi tcb = cb;
  cb = NULL;
  (*tcb) (status);

  // for those who add more code to this function ....bad idea; 
  // the call to (*tcb) might have very well deleted us; it's crucial
  // not to touch class variables after the (*tcb) call.
}

//-----------------------------------------------------------------------

void
http_parser_base_t::stop_abuf ()
{
  _abuf->finish ();
}

//-----------------------------------------------------------------------

void
http_parser_full_t::finish2 (int s1, int s2)
{
  if (s1 != HTTP_OK)
    finish (s1);
  else if (s2 != HTTP_OK)
    finish (s2);
  else
    finish (HTTP_OK);
}

//-----------------------------------------------------------------------

cbi::ptr
http_parser_full_t::prepare_post_parse (int status)
{
  if (hdr.contlen > int (ok_reqsize_limit)) {
    warn << "Header warns of upload too big (" 
         << hdr.contlen << "b vs. " << ok_reqsize_limit << "b); "
         << "aborting!\n";
    short_circuit_output ();
    finish (HTTP_NOT_ALLOWED);
    return NULL;
  } else if (hdr.contlen < 0)
    hdr.contlen = ok_reqsize_limit -1;
  
  _abuf->setlim (hdr.contlen);
  return wrap (this, &http_parser_full_t::finish2, status);
}

//-----------------------------------------------------------------------

tamed void
http_parser_full_t::parse_raw_body(int status)
{
    tvars {
        cbi::ptr pcb;
        ptr<async_raw_post_parser_t> post_parser;
        int ret;
    }
    pcb = prepare_post_parse(status);
    if (!pcb) return;

    post_parser = New refcounted<async_raw_post_parser_t>
        (get_abuf_p(), hdr.contlen);

    twait { post_parser->parse(mkevent(ret)); }
    m_raw_body = post_parser->contents();
    pcb(ret);
}

//-----------------------------------------------------------------------

void
http_parser_cgi_t::v_parse_cb1 (int status)
{
    if ((hdr.mthd == HTTP_MTHD_POST || hdr.mthd == HTTP_MTHD_PUT) &&
        want_raw_body())
    {
        cgi = get_url_p ();
        parse_raw_body(status);
        return;
    }

    switch (hdr.mthd) {

        case HTTP_MTHD_PUT:
        case HTTP_MTHD_POST:
            {
                cgi_t *post_cgi;
                ptr<cgi_t> ucg;

                if (_union_mode) {
                    ucg = get_union_cgi ();
                    cgi = ucg;
                    post_cgi = ucg;

                    // need to reset interior state for new parsing.
                    ucg->reset_state ();

                } else {
                    cgi = &post;
                    post_cgi = &post;
                }

                cbi::ptr pcb;
                if ((pcb = prepare_post_parse (status))) {

                    str boundary;
                    if (cgi_mpfd_t::match (hdr, &boundary)) {
                        if (mpfd_flag) {
                            mpfd = cgi_mpfd_t::alloc (_abuf, hdr.contlen, boundary);
                            cgi = mpfd;
                            mpfd->parse (pcb);
                        } else {
                            warn << "file upload attempted when not permitted\n";
                            finish (HTTP_NOT_ALLOWED);
                        }
                    } else {
                        post_cgi->set_max_scratchlen (hdr.contlen);
                        post_cgi->parse (pcb);
                    }
                }
            }
            break;

        case HTTP_MTHD_DELETE:
        case HTTP_MTHD_OPTIONS:
        case HTTP_MTHD_GET:
        case HTTP_MTHD_HEAD:
            {
                if (_union_mode) {
                    cgi = get_union_cgi ();
                } else {
                    cgi = get_url_p ();
                }
                finish (status);
            }
            break;

        default:
            {
                short_circuit_output ();
                finish (HTTP_NOT_ALLOWED);
            }
            break;
    }
}

//-----------------------------------------------------------------------

void
http_parser_base_t::clnt_timeout ()
{
  tocb = NULL;
  v_cancel ();
  short_circuit_output ();
  finish (v_timeout_status ());
}

//-----------------------------------------------------------------------

void
http_parser_cgi_t::set_union_mode (bool b)
{
  _union_mode = b;
  hdr.set_url (_union_mode ? get_union_cgi () : get_url_p ());
}

//-----------------------------------------------------------------------

void
http_parser_base_t::short_circuit_output ()
{
  if (_parser_x) _parser_x->short_circuit_output ();
}

//-----------------------------------------------------------------------

int
http_parser_cgi_t::v_timeout_status () const
{
  return (_parsing_header ? hdr.timeout_status () : HTTP_TIMEOUT);
}

//-----------------------------------------------------------------------

ptr<cgi_t>
http_parser_cgi_t::get_union_cgi ()
{
  if (!_union_cgi) {
    ptr<ok::scratch_handle_t> h = 
      ok::alloc_scratch (ok_http_inhdr_buflen_big);
    _union_cgi = cgi_t::alloc (get_abuf_p (), false, h);
  }
  return _union_cgi;
}

//-----------------------------------------------------------------------

http_parser_base_t::http_parser_base_t (ptr<ahttpcon> xx, u_int to, abuf_t *b)
  : _parser_x (xx), 
    _abuf (b ? b : New abuf_t (New abuf_con_t (xx), true)),
    _del_abuf (b ? false : true),
    timeout (to ? to : ok_clnt_timeout),
    tocb (NULL),
    destroyed (New refcounted<bool> (false)),
    _parsing_header (false),
    _scratch (ok::alloc_scratch (ok_http_inhdr_buflen_big)),
    _header_sz (0)
{
  assert (_abuf);
}

//-----------------------------------------------------------------------

http_parser_raw_t::http_parser_raw_t (ptr<ahttpcon> xx, u_int to, abuf_t *b)
  : http_parser_base_t (xx, to, b), 
    hdr (_abuf, _scratch) 
{
  assert (_abuf);
}

//-----------------------------------------------------------------------

http_parser_full_t::http_parser_full_t (ptr<ahttpcon> xx, u_int to, abuf_t *b)
  : http_parser_base_t (xx, to, b), 
    hdr (_abuf, _scratch) 
{
  assert (_abuf);
}

//-----------------------------------------------------------------------

http_parser_cgi_t::http_parser_cgi_t (ptr<ahttpcon> xx, int to, abuf_t *b) 
  : http_parser_full_t (xx, to, b),
    post (_abuf, false, _scratch),
    mpfd (NULL),
    mpfd_flag (false),
    _union_mode (false) 
{
  assert (_abuf);
}

//-----------------------------------------------------------------------
