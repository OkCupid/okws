
#include "mpfd.h"
#include "rxx.h"
#include "httpconst.h"
#include "pubutil.h"

static rxx multipart_rxx ("multipart/form-data[;,]\\s+boundary=(.*)");
mpfd_ktmap_t mpfd_ktmap;

bool
cgi_mpfd_t::match (const http_inhdr_t &hdr, str *bndry)
{
  if (multipart_rxx.match (hdr["content-type"])) {
    *bndry = multipart_rxx[1];
    return true;
  }
  return false;
}

cgi_mpfd_t *
cgi_mpfd_t::alloc (abuf_t *a, u_int len, const str &b)
{
  char *buf = (char *)xmalloc (len);
  cgi_mpfd_t *r = New cgi_mpfd_t (a, len, buf);
  r->add_boundary (b);
  return r;
}

#define MPFD_INC_STATE \
  state = static_cast<mpfdst_t> (state + 1)

void
cgi_mpfd_t::ext_parse_cb (int status)
{
  if (status == HTTP_OK) {
    MPFD_INC_STATE;
    resume ();
  } else {
    finish_parse (status);
  }
}

void
cgi_mpfd_t::parse_guts ()
{
  abuf_stat_t r = ABUF_OK;
  str dummy;
  
  bool inc;

  while (r == ABUF_OK) {

    inc = true;

    switch (state) {

    case MPFD_START:
      r = match_boundary ();
      break;

    case MPFD_EOL0:
      r = require_crlf ();
      break;

    case MPFD_KEY:
      r = gobble_crlf ();
      if (r == ABUF_OK) {
	if (to_start) {
	  state = MPFD_START;
	  to_start = false;
	} else 
	  state = MPFD_SEARCH;
	inc = false;
      } else if (r == ABUF_NOMATCH) {
	r = delimit_key (&mpfd_key);
	if (r == ABUF_OK)
	  kt = mpfd_ktmap.lookup (mpfd_key);
      } // else a WAIT or an EOF in gobble_crlf
      break;

    case MPFD_SPC:
      r = abuf->skip_hws (1);
      cdp.reset ();
      break;

    case MPFD_VALUE:
      if (kt == MPFD_DISPOSITION) {
	cdp.parse (wrap (this, &cgi_mpfd_t::ext_parse_cb));
	return;
      }	else if (kt == MPFD_TYPE) {
	r = delimit_val (&content_typ);
	if (r == ABUF_OK) {
	  if (multipart_rxx.match (content_typ)) {
	    add_boundary (multipart_rxx[1]);
	    to_start = true;
	  }
	}
      } else {
	r = delimit_val (&dummy);
      }
      break;

    case MPFD_EOL1A:
      r = require_crlf ();
      break;

    case MPFD_EOL1B:
      if (kt == MPFD_DISPOSITION) {
	if (cdp.typ == CONTDISP_FORMDAT) {
	  cgi_key = cdp.name;
	  filename = cdp.filename;
	  attach = filename;
	} else if (cdp.typ == CONTDISP_ATTACH) {
	  filename = cdp.filename;
	  attach = true;
	} else {
	  r = ABUF_PARSE_ERR;
	}
      }
      state = MPFD_KEY;
      inc = false;
      break;

    case MPFD_SEARCH:
      r = match_boundary (&dat);
      if (r == ABUF_OK) {
	if (cgi_key) {
	  if (attach)
	    finsert (cgi_key, cgi_file_t (filename, content_typ, dat));
	  else
	    insert (cgi_key, dat);
	  cgi_key = NULL;
	}
	// in this case, no more boundaries
      } else if (r == ABUF_PARSE_ERR) { 
	r = ABUF_OK;
	state = MPFD_EOF;
	inc = false;
      }
      break;

    case MPFD_SEARCH2:
      r = parse_2dash ();
      if (r == ABUF_OK) {
	remove_boundary ();
	nxt_state = MPFD_SEARCH;
      } else if (r == ABUF_NOMATCH) {
	r = ABUF_OK;
	nxt_state = MPFD_KEY;
      }
      break;

    case MPFD_SEARCH3:
      r = require_crlf ();
      if (r == ABUF_OK) {
	state = nxt_state;
	inc = false;
      }
      break;

    case MPFD_EOF:
      r = abuf->skip_ws ();
      break;

    default:
      break;

    }
    if (r == ABUF_OK && inc)
      MPFD_INC_STATE;
  }

  switch (r) {
  case ABUF_EOF:
    finish_parse (state == MPFD_EOF ? HTTP_OK : HTTP_UNEXPECTED_EOF);
    break;
  case ABUF_PARSE_ERR:
    finish_parse (HTTP_BAD_REQUEST);
    break;
  default:
    break;
  }
}

abuf_stat_t
cgi_mpfd_t::parse_2dash ()
{
  int ch;
  abuf_stat_t r = ABUF_OK;
  int ndash = 0;
  while (r == ABUF_OK && ndash < 2) {
    ch = abuf->get ();
    switch (ch) {
    case '-':
      ndash++;
      break;
    case ABUF_EOFCHAR:
      r = ABUF_EOF;
      break;
    case ABUF_WAITCHAR:
      r = ABUF_WAIT;
      break;
    default:
      r = ABUF_NOMATCH;
      abuf->unget ();
      break;
    }
  }
  return r;
}

bool
cgi_mpfd_t::flookup (const str &k, cgi_files_t **v)
{
  pair_t *p = tab[k];
  if (!p)
    return false;
  cgi_mpfd_pair_t *cp = p->to_cgi_mpfd_pair ();
  assert (cp);
  if (!cp->files)
    return false;
  *v = cp->files;
  return true;
}

void
cgi_mpfd_t::finsert (const str &k, const cgi_file_t &f)
{
  pair_t *p = tab[k];
  cgi_mpfd_pair_t *cp;
  if (p) {
    cp = p->to_cgi_mpfd_pair ();
    assert (cp);
    cp->add (f);
  } else {
    cp = New cgi_mpfd_pair_t (k);
    cp->add (f);
    insert (cp);
  }
}

void
cgi_mpfd_pair_t::add (const cgi_file_t &f)
{
  if (!files)
    files = New cgi_files_t ();
  files->push_back (f);
}

void
cgi_mpfd_t::add_boundary (const str &b)
{
  boundaries.push_back (New kmp_matcher_t (strbuf ("--") << b));
}

abuf_stat_t
cgi_mpfd_t::match_boundary (str *dat)
{
  if (!boundaries.size ())
    return ABUF_PARSE_ERR;

  if (cbm != boundaries.back () || match_ended) {
    match_ended = false;
    cbm = boundaries.back ();
    cbm->reset ();
    if (dat)
      abuf->mirror (buf, buflen);
  }

  int ch;
  bool flag = true;
  while (flag) {
    ch = abuf->get ();
    if (IS_CONTROL_CHAR (ch)) { 
      flag = false;
    } else {
      if (cbm->match (ch)) {
	if (dat)
	  *dat = abuf->end_mirror2 (cbm->len ());
	match_ended = true;
	return ABUF_OK;
      }
    }
  }
  if (ch == ABUF_WAITCHAR) {
    return ABUF_WAIT; 
  } else {
    match_ended = true;
    return ABUF_EOF;
  }
}

void
cgi_mpfd_t::remove_boundary ()
{
  if (boundaries.size ()) 
    delete boundaries.pop_back ();
  cbm = NULL;
}

mpfd_ktmap_t::mpfd_ktmap_t ()
{
  map.insert ("content-disposition", MPFD_DISPOSITION);
  map.insert ("content-type", MPFD_TYPE);
}

mpfdkt_t
mpfd_ktmap_t::lookup (const str &s) const
{
  const mpfdkt_t *v = map[s];
  if (!v)
    return MPFD_OTHER;
  return *v;
}

cgi_mpfd_t::~cgi_mpfd_t ()
{
  xfree (buf);
  while (boundaries.size ())
    remove_boundary ();
}

void
contdisp_parser_t::parse_guts ()
{
  abuf_stat_t r = ABUF_OK;
  bool inc;
  bool flag = true;

  while (r == ABUF_OK && flag) {
    inc = true;
    switch (state) {
    case CONTDISP_START:
      r = delimit (&typ_scr, ';', true, true);
      break;
    case CONTDISP_SEP1:
      r = abuf->skip_hws (1);
      if (mystrcmp (typ_scr, "form-data")) {
	typ = CONTDISP_FORMDAT;
      } else if (mystrcmp (typ_scr, "attachment")) {
	state = CONTDISP_FILENAME_KEY;
	inc = true;
	typ = CONTDISP_ATTACH;
      }
      break;
    case CONTDISP_NAME_KEY:
      r = force_match ("name=\"");
      break;
    case CONTDISP_NAME_VAL:
      r = delimit (&name, '"', false, true);
      break;
    case CONTDISP_SEP2A:
      r = eol ();
      if (r == ABUF_OK) flag = false;
      else if (r == ABUF_NOMATCH) r = ABUF_OK;
      break;
    case CONTDISP_SEP2B:
      r = abuf->requirechar (';');
      break;
    case CONTDISP_SEP2C:
      r = abuf->skip_hws ();
      break;
    case CONTDISP_FILENAME_KEY:
      r = force_match ("filename=\"");
      break;
    case CONTDISP_FILENAME_VAL:
      r = delimit (&filename_scr, '\r', false, false);
      if (r == ABUF_PARSE_ERR)
	r = ABUF_OK; // found '\n' before '\r' -- that's OK
      if (r == ABUF_OK) {
	filename = str (filename_scr.cstr (), filename_scr.len () - 1);
	flag = false;
      }
      break;
    default:
      break;
    }
    if (r == ABUF_OK && inc)
      state = static_cast<contdisp_state_t> (state + 1);
  }

  switch (r) {
  case ABUF_OK:
    finish_parse (HTTP_OK);
    break;
  case ABUF_EOF:
  case ABUF_PARSE_ERR:
    finish_parse (HTTP_BAD_REQUEST);
    break;
  default:
    break;
  }
}


void 
contdisp_parser_t::reset ()
{
  http_hdr_t::reset ();
  typ = CONTDISP_NONE;
  name = NULL;
  filename = NULL;
  state = CONTDISP_START;
}
