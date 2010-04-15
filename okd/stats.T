// -*-c++-*-

#include "okd.h"

//-----------------------------------------------------------------------

tamed void
okch_t::stats_collect (okd_stats_t *s, evv_t ev)
{
  tvars {
    oksvc_stats_t resp;
    clnt_stat err;
  }

  if (ctlx) {
    twait {
      RPC::okctl_program_1::okctl_get_stats_from_svc 
	(clnt, &resp, mkevent (err)); 
    }
    if (err) {
      warn << _servpath << ": error in collecting stats for service: "
	   << err << "\n";
    } else {
      s->_n_recv += resp.n_recv;
      s->_n_sent += resp.n_sent;
      s->_n_tot += (resp.n_recv + resp.n_sent);
    }
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

tamed void
okd_t::stats_collect (okd_stats_t *s, evv_t ev)
{
  tvars {
    okch_t *p;
    vec<okch_t *> all;
    size_t i;
  }

  servtab.dump (&all);

  s->_n_recv = s->_n_sent = s->_n_tot = 0;

  twait { 
    for (i = 0; i < all.size (); i++) {
      p = all[i];
      p->stats_collect (s, mkevent ());
    }
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

tamed void
okd_t::render_stats_page (ptr<ahttpcon_clone> x, evv_t ev)
{
  tvars {
    ptr<http_parser_raw_t> prs;
    int status;
    htpv_t v;
    okd_stats_t stats;
    http_inhdr_t *h (NULL);
  }
  x->declone ();

  if (!x->closed ()) {
    prs = http_parser_raw_t::alloc (x);
    twait { prs->parse (mkevent (status)); }
    h = prs->hdr_p ();
  }

  twait { stats_collect (&stats, mkevent ()); }
  stats._uptime = sfs_get_timenow () - _startup_time;
  stats._n_req = reqid;

  v = h ? h->get_vers () : 0;

  twait { send_stats_reply (x, stats, v, mkevent ()); }
  ev->trigger ();
}

//-----------------------------------------------------------------------

tamed void
okd_t::send_stats_reply (ptr<ahttpcon> x, const okd_stats_t &stats, htpv_t v,
			 evv_t ev)
{
  tvars {
    strbuf b;
    ptr<http_response_ok_t> resp;
    http_resp_attributes_t hra (HTTP_OK, v);
  }

  stats.to_strbuf (b);
  hra.set_content_type ("text/plain");
  resp = New refcounted<http_response_ok_t> (b, hra);
  if (!x->closed ()) {
    twait { resp->send (x, mkevent ()); }
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

#define B2K(x) ((x) >> 10)

void
okd_stats_t::to_strbuf (strbuf &b) const
{

  b << "Total Accesses: " << _n_req << "\n"
    << "Total kBytes: " << B2K(_n_tot) << "\n"
    << "Uptime: " << _uptime << "\n"
    << "Total Read kBytes: " << B2K(_n_recv) << "\n"
    << "Total Send kBytes: " << B2K(_n_sent) << "\n";
}

#undef B2K

//-----------------------------------------------------------------------
