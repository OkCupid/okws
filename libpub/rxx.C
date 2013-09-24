#include "async.h"
#include "okws_rxx.h"

//-----------------------------------------------------------------------

str
rxx_replace (str input, rxx pattern, str repl)
{
  str ret;
  if (input) {
    vec<str> v;
    split (&v, pattern, input, size_t (-1), true);
    ret = join (repl, v);
  }
  return ret;
}

//-----------------------------------------------------------------------

// A lot of this is taken from async/rxx.C in sfslite, but with some
// modifications for returning the matched pattern.
//
int
split2 (vec<str> *out, rxx pat, str expr, size_t lim, bool emptylast)
{
  const char *p = expr.cstr();
  const char *const e = p + expr.len ();
  size_t n;
  if (out)
    out->clear ();

  // check p < e to see that we're not dealing with an empty
  // string (especially since x? matches "").
  for (n = 0; p < e && n + 1 < lim; n++) {
    if (!pat._exec (p, e - p, 0)) {
      return 0;
    }
    if (!pat.success ())
      break;
    if (out) {
      out->push_back (str (p, pat.start (0)));
      str sep (p + pat.start (0), pat.len (0));
      out->push_back (sep);
    }
    p += max (pat.end (0), 1);
  }

  if (lim && (p < e || emptylast)) {
    n++;
    if (out) {
      out->push_back (str (p, e - p));
    }
  }
  return n;
}

//=======================================================================

static void strbuf_output (strbuf &b, str s) { b << s; b.hold_onto (s); }

//-----------------------------------------------------------------------

struct repl_el_t {
  virtual ~repl_el_t() { }
  virtual void output (strbuf &out, const char *s, rxx &x) = 0;
};

//-----------------------------------------------------------------------

struct repl_el_str_t : public repl_el_t {
  repl_el_str_t (str s) : _s (s) {}
  void output (strbuf &out, const char *s, rxx &x);
  str _s;
};

//-----------------------------------------------------------------------

struct repl_el_capture_t : public repl_el_t {
  repl_el_capture_t (size_t i) : _i (i) {}
  void output (strbuf &out, const char *s, rxx &x);
  size_t _i;
};

//-----------------------------------------------------------------------

class repl_t {
public:
  repl_t () {}
  ~repl_t () { clear(); }
  bool parse (str in);
  void output (strbuf &out, const char *s, rxx &x);
private:
  void clear ();
  void add_str (const char *bb, const char *bs, const char *es, const char *eb);
  void add_capture (size_t i);
  vec<repl_el_t *> _els;
};

//-----------------------------------------------------------------------

void 
repl_el_str_t::output (strbuf &out, const char *s, rxx &x)
{
  if (_s && _s.len ()) { strbuf_output (out, _s); }
}

//-----------------------------------------------------------------------

void
repl_t::output (strbuf &out, const char *s, rxx &x)
{
  for (size_t i = 0; i < _els.size (); i++) {
    _els[i]->output (out, s, x);
  }
}

//-----------------------------------------------------------------------

void
repl_el_capture_t::output (strbuf &out, const char *s, rxx &x)
{
  int start = x.start (_i);
  int ln = x.len (_i);

  if (start >= 0 && ln > 0) {
    str repl = str (s + start, ln);
    strbuf_output (out, repl); 
  }
}

//-----------------------------------------------------------------------

void
repl_t::clear ()
{
  for (size_t i = 0; i < _els.size (); i++) { delete _els[i]; }
  _els.setsize (0);
}

//-----------------------------------------------------------------------

void repl_t::add_capture (size_t i) 
{ _els.push_back (New repl_el_capture_t (i)); }

//-----------------------------------------------------------------------

void
repl_t::add_str (const char *bb, const char *bs, const char *es, const char *eb)
{
  const char *b = max<const char *> (bb, bs);
  const char *e = min<const char *> (es, eb);
  
  if (e > b) {
    _els.push_back (New repl_el_str_t (str (b, e - b)));
  }
}

//-----------------------------------------------------------------------

bool
repl_t::parse (str in)
{
  const char *bp = in.cstr ();
  const char *ep = bp + in.len ();
  const char *last = bp;
  size_t inc = 1;

  for (const char *p = bp; p < ep; p += inc) {
    bool is_dig = false;
    inc = 1;
    
    if (p == ep - 1 || *p != '$') { /* noop */ }

    else if (p[1] == '$' || (is_dig = (isdigit (p[1])))) {

      add_str (bp, last, p, ep);
      inc = 2;

      if (is_dig) {
	add_capture (p[1] - '0');
	last = p + 2;
      } else {
	last = p + 1;
      }
    }

  }
  add_str (bp, last, ep, ep);
  return true;
}

//-----------------------------------------------------------------------

str
rxx_replace_2 (str input, rxx pat, str repl_str)
{
  repl_t repl;
  str ret;
  if (!repl.parse (repl_str)) {
    warn << "XX cannot parse replacement string: " << repl_str << "\n";
  } else {
    const char *p = input.cstr();
    const char *const e = p + input.len ();
    strbuf b;
    bool go = true;
    bool err = false;

    // check p < e to see that we're not dealing with an empty
    // string (especially since x? matches "").
    while (go && !err && p < e) {

      if (!pat._exec (p, e - p, 0)) { 
	warn << "XX regex execution failed\n";
	err = true; 
      }
      
      else if (!pat.success ()) { go = false; }

      else {
	str pre = str (p, pat.start (0));
	strbuf_output (b, pre);
	repl.output (b, p, pat);
	p += max (pat.end (0), 1);
      }

    }

    if (p < e && !err) {
      str post = str (p, e - p);
      strbuf_output (b, post);
    }

    if (!err) { ret = b; }

  }
  return ret;
}

//-----------------------------------------------------------------------

static void
extract_matches (vec<str> *out, const char *base, rxx &x)
{
  bool go = true;
  for (int i = 0; go; i++) {

    int ln = x.len (i);
    int start = x.start (i);

    if (ln < 0 || start < 0) { go = false; }
    else if (ln > 0) { out->push_back (str (base + start, ln)); }
  }
}

//-----------------------------------------------------------------------

// Call the replace function for each matched pattern.
str 
rxx_replace (str input, rxx pat, rxx_replace_cb_t cb)
{
  const char *p = input.cstr();
  const char *const e = p + input.len ();
  strbuf b;
  bool go = true;
  bool err = false;
  str ret;

  // check p < e to see that we're not dealing with an empty
  // string (especially since x? matches "").
  while (go && !err && p < e) {
    
    if (!pat._exec (p, e - p, 0)) { 
      warn << "XX regex execution failed\n";
      err = true; 
    }
    
    else if (!pat.success ()) { go = false; }
    
    else {
      str pre = str (p, pat.start (0));
      strbuf_output (b, pre);
      vec<str> v;
      extract_matches (&v, p, pat);
      str repl = (*cb) (&v);
      if (repl) {
	strbuf_output (b, repl);
      }
      p += max (pat.end (0), 1);
    }
    
  }
  
  if (p < e && !err) {
    str post = str (p, e - p);
    strbuf_output (b, post);
  }
  
  if (!err) { ret = b; }
  return ret;
}

//-----------------------------------------------------------------------
