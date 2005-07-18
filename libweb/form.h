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


#ifndef _LIBWEB_FORM_H
#define _LIBWEB_FORM_H

#include "str.h"
#include "cgi.h"
#include "pubutil.h"
#include "web.h"
#include "parseopt.h"
#include "pub.h"

class webform_t;
class wfel_t {
public:
  wfel_t (webform_t *w, const str &n, const str &l) 
    : wf (w), name (n), label (l), err (false) {}
  wfel_t () : err (false) {}
  virtual ~wfel_t () {}
  virtual str process () = 0;
  virtual str output_field () = 0;
  virtual str output_label () const;
  virtual bool getval (str *s) const { return false; }
  virtual bool getval (int64_t *i) const { return false; }
  template<typename T> bool getval (T *i) const;

  virtual bool getval (vec<str> *v) const { return false; }
  virtual void setval (const str &s) {}
  virtual void setval (int64_t i) {}
  virtual void getval (cgi_t *c) = 0;
  virtual str add_error (const str &in);
  virtual void clear () {}
  virtual void fill (aarr_t *a);
  virtual bool is_submit () const { return false; }

  webform_t *wf;
  const str name;
  const str label;
  bool err;
  str err_str;
  ihash_entry<wfel_t> lnk;
};


class wfel_ival_t : public wfel_t {
public:
  wfel_ival_t (webform_t *u, const str &n, const str &l)
    : wfel_t (u, n, l), ival_ok (false) {}
  virtual bool getval (str *s) const { *s = sval; return true; }
  virtual bool getval (int64_t *i) const;
  virtual void setval (const str &s) { sval = s; }
  virtual void setval (int64_t i) 
  { sval = strbuf () << i; ival = i; ival_ok = true; }
  virtual void clear () { sval = NULL; ival_ok = false; }
  virtual void set_ival ();
protected:
  str sval;
  int64_t ival;
  bool ival_ok;
};

class wfel_ivar_t : public wfel_ival_t {
public:
  template<typename T>
  wfel_ivar_t (webform_t *w, const str &n, T v)
    : wfel_ival_t (w, n, NULL) { setval (v); }
  str process () { return NULL; }
  str output_field () { str s; return wfel_ival_t::getval (&s) ? s : sNULL; }
  str output_label () { return NULL; }
  void getval (cgi_t *c) {}
};

typedef callback<str, wfel_t *>::ref wfpcb_t;

class wfel_text_t : public wfel_ival_t {
public:
  wfel_text_t (webform_t *w, const str &n, const str &l, 
	       wfpcb_t c, int s = -1, int ml = -1)
    : wfel_ival_t (w, n, l), pcb (c), size (s), maxlen (ml) {}
  virtual ~wfel_text_t () {}
  virtual void getval (cgi_t *c);
  str output_field ();
  str process () { return add_error ((*pcb) (this)); }
protected:
  wfpcb_t pcb;
  int size;
  int maxlen;
};

class wfel_int_t : public wfel_ival_t {
public:
  wfel_int_t (webform_t *w, const str &n, const str &l, int64_t mn, 
	      int64_t mx, int s = -1)
    : wfel_ival_t (w, n, l), minval (mn), maxval (mx), 
      size (s < 0 ? my_log_10 (max (abs (mn), abs (mx)) + 2) : s) {}

  virtual void getval (cgi_t *c);
  virtual bool getval (str *s) const;
  virtual void setval (const str &s) { ival_ok = convertint (s, &ival);}
  virtual void setval (int64_t i) { ival = i; ival_ok = true; }
  void clear () { ival_ok = false; }
  str process ();
  str output_field ();
private:
  int64_t minval, maxval;
  int size;
};

class wf_menu_pair_t {
public:
  wf_menu_pair_t (const str &v, const str &l = NULL, bool sel = false) 
    : value (v), label (l ? l : v), selected (sel) {}
  void output (strbuf &b, bool hasval);
  const str value;
  const str label;
  bool selected;
  ihash_entry<wf_menu_pair_t> lnk;
};

class wfel_menu_t : public wfel_ival_t {
public:
  wfel_menu_t (webform_t *w, const str &n, const str &l) 
    : wfel_ival_t (w, n, l) {}
  ~wfel_menu_t ();
  void insert (wf_menu_pair_t *p);
  void insert (const str &v, const str &l = NULL, bool sel = false) 
  { insert (New wf_menu_pair_t (v, l, sel)); if (sel) def = v;}

  str output_field ();
  virtual void getval (cgi_t *c);
  virtual str process ();
  void clear ();
  void set_select (const str &v, bool b);
  
private:
  str def;
  ihash<const str, wf_menu_pair_t, &wf_menu_pair_t::value, 
	&wf_menu_pair_t::lnk> tab;
  vec<wf_menu_pair_t *> els;
};

class wfel_submit_t : public wfel_t {
public:
  wfel_submit_t (webform_t *w, const str &n, const str &v)
    : wfel_t (w, n, NULL), value (v), sbmt_flag (false) {}
  str output_field ();
  void getval (cgi_t *c) { sbmt_flag = ((*c)[name] == value); }
  bool submitted () const { return sbmt_flag; }
  str process () { return NULL; }
  void clear () { sbmt_flag = false; }
  str output_label () const { return NULL; }
  bool is_submit () const { return true; }
private:
  const str value;
  bool sbmt_flag;
};

class webform_t {
public:
  webform_t (const str &p) : prefix (p) {}
  virtual ~webform_t ();

  webform_t &add_email (int s = 80, int ml = 200);
  webform_t &add_name (int s = 80, int ml = 200);
  webform_t &add_yob ();
  webform_t &add_zip ();
  webform_t &add_sex ();
  webform_t &add_submit ();
  webform_t &add (wfel_t *e);
  webform_t &add_submit (wfel_submit_t *e);

  template<typename T> webform_t &add_ivar (const str &n, T t);
  webform_t &add_text (const str &n, const str &l, wfpcb_t c, 
		       int s = -1, int ml = -1);
  webform_t &add_int (const str &n, const str &l, int64_t mn, int64_t mx, 
		      int s = -1);

  virtual str name_error (const str &in) const;
  str rename (const str &i) const;
  void set_cgi (cgi_t *c) { cgip = c; }
  bool submitted () const;
  bool process ();
  void clear ();
  void fill (aarr_t *a);
  void add_error (const str &s) { errors.push_back (s); }
  void readvals (bool excl_sbmt);
  
  template<class C> bool lookup (const str &n, C *v) const;
  str safe_lookup (const str &n) const;
  str operator[] (const str &n) const { return safe_lookup (n); }

  str error ();

private:
  cgi_t *cgip;
  const str prefix;
  ihash<const str, wfel_t, &wfel_t::name, &wfel_t::lnk> tab;
  vec<wfel_t *> els;
  vec<wfel_submit_t *> submits;
  vec<str> errors;
};


template<class C> bool 
webform_t::lookup (const str &n, C *v) const
{
  if (!n) return false;
  wfel_t *el = tab[rename (n)];
  if (!el) return false;
  return el->getval (v);
}

template<typename T> bool
wfel_t::getval (T *i) const
{
  int64_t h;
  if (getval (&h)) {
    *i = h;
    return true;
  }
  return false;
}

template<typename T> webform_t &
webform_t::add_ivar (const str &n, T t)
{
  wfel_ivar_t *e = New wfel_ivar_t (this, rename (n), t);
  add (e);
  return (*this);
}


str check_email (const str &in);
str check_zipcode (const str &in);

#endif /* _LIBWEB_FORM_H */
