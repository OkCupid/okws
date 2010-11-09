// -*-c++-*-
/* $Id: pubutil.h 4155 2009-02-27 14:58:37Z max $ */


/*
 *
 * Copyright (C) 2003 Maxwell Krohn (max@okcupid.com)
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

#ifndef _LIBPUB_PESCAPE_H 
#define _LIBPUB_PESCAPE_H 1

#include "async.h"
#include "qhash.h"
#include "rxx.h"

str json_escape (const str &s, bool qs, bool utf8 = false);
str xss_escape (const char *s, size_t l);
str xss_escape (const str &s);

//-----------------------------------------------------------------------

class filter_buf_t {
public:
  filter_buf_t () {}
  str to_str () { return _b; }
  void add_s (str s) { _hold.push_back (s); _b << s; }
  void add_ch (char ch);
  void add_cc (const char *p, ssize_t len = -1, bool cp = false);
private:
  strbuf _b;
  vec<str> _hold;
};

//-----------------------------------------------------------------------

class html_filter_t {
public:

  str run (const str &in);
  virtual ~html_filter_t () {}

protected:
  html_filter_t () {}
  typedef filter_buf_t buf_t;

  static bool find_space_in (const char *start, const char *end);
  static const bhash<str> &safe_entity_list ();
  static bool is_safe_entity (const char *start, const char *end);

protected:
  virtual void handle_tag (buf_t *out, const char **cpp, const char *ep) = NULL;
};

//-----------------------------------------------------------------------

class html_filter_rxx_t : public html_filter_t {
public:
  html_filter_rxx_t (ptr<rxx> x) : _rxx (x) {}
  void handle_tag (buf_t *out, const char **cpp, const char *ep);
private:
  ptr<rxx> _rxx;
};

//-----------------------------------------------------------------------

class html_filter_bhash_t : public html_filter_t {
public:
  html_filter_bhash_t (ptr<const bhash<str> > bh) : _tab (bh) {}
protected:
  void handle_tag (buf_t *out, const char **cpp, const char *ep);
private:
  bool match (const char *start, const char *end) const;
  ptr<const bhash<str> > _tab;
};

//-----------------------------------------------------------------------

// modelled after php's htmlspecialchars
str htmlspecialchars (const str &in);

//-----------------------------------------------------------------------

#endif /* _LIBPUB_PESCAPEL_H */
