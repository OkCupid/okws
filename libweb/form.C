#include "form.h"

str
wfel_t::output_label () const
{
  if (!label)
    return NULL;
  return err ? wf->name_error (label) : label;
}

str
webform_t::name_error (const str &in) const
{
  strbuf b;
  b << "<font color=red><b>" << in << "</b></font>";
  return b;
}

static str
output_field (const str &nm, const str &sval, int mxln, int sz)
{
  strbuf b;
  b << "<input type=text name=";
  b.cat (c_escape (nm), true);
  if (sz > 0) 
    b << " size=" << sz;
  if (mxln > 0)
    b << " maxlength=" << mxln;
  if (sval) {
    b << " value=";
    b.cat (c_escape (sval), true);
  }
  b << ">";
  return b;
}

str
wfel_text_t::output_field ()
{
  return ::output_field (wfel_ival_t::name, sval, maxlen, size);
}

str
wfel_int_t::output_field ()
{
  return ::output_field (name, ival_ok ? str (strbuf () << ival) : sNULL, 
			 size, size);
}

str
wfel_menu_t::output_field ()
{
  strbuf b;
  b << "<select name=" << c_escape (name) << ">\n";
  u_int lim = els.size ();
  for (u_int i = 0; i < lim; i++) {
    els[i]->output (b, sval);
  }
  b << "</select>\n";
  return b;
}

void
wfel_menu_t::readval (cgi_t *c)
{
  wf_menu_pair_t *p = NULL;
  if (!c->lookup (name, &sval) || !sval || !(p = tab[sval]))
    sval = NULL;
  else 
    set_ival ();

  if (p)
    p->selected = true;
  set_select (def, false);
}

void
wfel_menu_t::clear ()
{
  set_select (sval, false);
  set_select (def, true);
}

void
wfel_menu_t::set_select (const str &n, bool b)
{
  if (n) {
    wf_menu_pair_t *p = tab[n];
    if (p) p->selected = b;
  }
}

void
wf_menu_pair_t::output (strbuf &b, bool hasval)
{
  b << "<option" ;
  if (value) {
    b << " value=";
    b.cat (c_escape (value), true);
  }
  if (selected && (!hasval || value))
    b << " selected";
  b << ">" << label;
  b << "\n";
}

void
wfel_int_t::readval (cgi_t *c)
{
  ival_ok = c->lookup (name, &ival);
}

str
wfel_int_t::process () 
{
  str res;
  if (!ival_ok) {
    res = strbuf ("You must enter a value for <b>") << label << "</b>";
  } else if (ival < minval || ival > maxval) {
    res = strbuf ("Your <b>") << label << "</b> must be between "
			      << minval << " and " << maxval;
  }
  return add_error (res);
}

str
wfel_menu_t::process ()
{
  str res;
  if (!sval) {
    res = strbuf ("No value selected for <b>") << label << "</b>";
  }
  return add_error (res);
}
    
str
wfel_t::add_error (const str &i)
{
  if (i) {
    err = true;
    err_str = i;
  }
  return i;
}

void
wfel_text_t::readval (cgi_t *c)
{
  c->lookup (wfel_ival_t::name, &sval);
  set_ival ();
}

void
wfel_ival_t::set_ival ()
{
  ival_ok = (sval && convertint (sval, &ival));
}

str
webform_t::rename (const str &i) const
{
  strbuf b;
  b << prefix << "_" << i;
  return b;
}

webform_t &
webform_t::add_text (const str &n, const str &l, wfpcb_t c, int s, int ml)
{
  wfel_text_t *e = New wfel_text_t (this, rename (n), l, c, s, ml);
  return add (e);
}

webform_t &
webform_t::add_int (const str &n, const str &l, int64_t mnv, int64_t mxv,
		    int s)
{
  wfel_int_t *e = 
    New wfel_int_t (this, rename (n), l, mnv, mxv, s);
  return add (e);
}

webform_t &
webform_t::add (wfel_t *e)
{
  tab.insert (e);
  els.push_back (e);
  return (*this);
}

static str
wf_email_cb (wfel_t *e)
{
  str s;
  str s2;
  if (!e->getval (&s) || !s.len ())
    return "You didn't enter your <b>e-mail address</b>";
  if (!(s2 = check_email (s))) 
    return strbuf ("<b>") << s << "</b> is not a valid e-mail address";
  e->setval (s2);
  return NULL;
}

static str
wf_name_cb (wfel_t *e)
{
  str s;
  if (!e->getval (&s) || !s.len ())
    return "You didn't enter your <b>name</b>";
  if (s.len () < 2)
    return strbuf (s) << ": your name is longer than that";
  return NULL;
}

static str
wf_zip_cb (wfel_t *e)
{
  str s, s2, ret;
  if (!e->getval (&s) || !s.len ())
    ret = "No <b>zipcode</b> specified";
  else if (!(s2 = check_zipcode (s))) 
    ret = strbuf ("<b>") << s << "</b> is not a valid zipcode";
  return ret;
}

webform_t &
webform_t::add_email (int s, int ml)
{
  return add_text ("email", "E-mail Address", wrap (wf_email_cb), s, ml);
}

webform_t &
webform_t::add_name (int s, int ml)
{
  return add_text ("name", "Full Name", wrap (wf_name_cb), s, ml);
}

webform_t &
webform_t::add_yob ()
{
  return add_int ("yob", "Year Of Birth", 1900, 2000, 4);
}

webform_t &
webform_t::add_zip ()
{
  return add_text ("zip", "Zip Code", wrap (wf_zip_cb), 10, 10);
}

webform_t::~webform_t ()
{
  for (u_int i = 0; i < els.size (); i++) {
    delete els[i];
  }
}

wfel_menu_t::~wfel_menu_t ()
{
  u_int lim = els.size ();
  for (u_int i = 0; i < lim; i++) 
    delete els[i];
}

webform_t &
webform_t::add_sex ()
{
  wfel_menu_t *m = New wfel_menu_t (this, rename ("sex"), "Sex");
  m->insert (NULL , "- select -", true);
  m->insert ("f", "female");
  m->insert ("m", "male");
  return add (m);
}

webform_t &
webform_t::add_submit ()
{
  str n = rename ("submit");
  wfel_submit_t *m = New wfel_submit_t (this, n, "submit");
  return add_submit (m);
}

void 
wfel_menu_t::insert (wf_menu_pair_t *p) 
{ 
  if (p->selected) def = p->value;
  if (p->value) tab.insert (p); 
  els.push_back (p); 
}

str
wfel_submit_t::output_field ()
{
  strbuf b;
  b << "<input type=submit name=";
  b.cat (c_escape (name), true);
  if (value) {
    b << " value=";
    b.cat (c_escape (value), true);
  }
  b << ">";
  return b;
}

webform_t &
webform_t::add_submit (wfel_submit_t *e)
{
  els.push_back (e);
  tab.insert (e);
  submits.push_back (e);
  return (*this);
}

void
webform_t::readvals (bool exclude_submits)
{
  u_int lim = els.size ();
  for (u_int i = 0; i < lim; i++) 
    if (!exclude_submits || !els[i]->is_submit ())
      els[i]->readval (cgip);
}

bool
webform_t::submitted () const
{
  u_int lim = submits.size ();
  for (u_int i = 0; i < lim; i++) {
    submits[i]->readval (cgip);
    if (submits[i]->submitted ())
      return true;
  }
  return false;
}

bool
webform_t::process () 
{
  readvals (true);
  bool ret = true;
  u_int lim = els.size ();
  for (u_int i = 0; i < lim ; i++) {
    str s = els[i]->process ();
    if (s) {
      errors.push_back (s);
      ret = false;
    }
  }
  return ret;
}

void
webform_t::clear ()
{
  u_int lim = els.size ();
  for (u_int i = 0; i < lim; i++)
    els[i]->clear ();
}

str
webform_t::error ()
{
  u_int lim = errors.size ();
  if (lim == 0)
    return NULL;
  
  strbuf b;
  for (u_int i = 0; i < lim; i++) {
    b << "<li>" << errors[i] << "</i>\n";
  }
  return b;
}

void
webform_t::fill (aarr_t *a)
{
  u_int lim = els.size ();
  for (u_int i = 0; i < lim; i++) {
    els[i]->fill (a);
  }
}

void
wfel_t::fill (aarr_t *a)
{
  str lv = output_label ();
  str fv = output_field ();
  if (lv) {
    strbuf b;
    b << name << "_l";
    a->add (b, lv);
  }
  if (fv) {
    strbuf b;
    b << name << "_f";
    a->add (b, fv);
  }
}

bool
wfel_ival_t::getval (int64_t *i) const
{
  if (ival_ok) {
    *i = ival;
    return true;
  }
  return false;
}

bool
wfel_int_t::getval (str *s) const
{
  if (ival_ok) {
    *s = strbuf () << ival;
    return true;
  }
  return false;
}

str
webform_t::safe_lookup (const str &n) const
{
  str s;
  if (!lookup (n, &s))
    s = "";
  return s;
}
