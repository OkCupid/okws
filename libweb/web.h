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

#ifndef _LIBWEB_WEB_H
#define _LIBWEB_WEB_H

#include "str.h"
#include "web_prot.h"
#include <time.h>

#define OK_DATE (1 << 0)
#define OK_TIME (1 << 1)

class okdate_t {
public:
  okdate_t (const x_okdate_t &x) : err (false), dt_tm (0), stm_set (false),
				   time_t_set (false)
  { set (x); }

  okdate_t (const x_okdate_time_t &x) : err (false), dt_tm (OK_TIME), 
    stm_set (false), time_t_set (false) { set (x); }
  okdate_t (const x_okdate_date_t &x) : err (false), dt_tm (OK_DATE), 
    stm_set (false), time_t_set (false) { set (x); }

  okdate_t (const struct tm &s) 
    : err (false), dt_tm (0), stm_set (false), time_t_set (false)
  { set (s); }

  okdate_t (time_t tm) 
    : err (false), dt_tm (0), stm_set (false), time_t_set (true) 
  { set (tm); }

  okdate_t (const str &s) 
    : err (false), dt_tm (0), stm_set (false), time_t_set (false)
  { set (s); }

  okdate_t (int month, int day, int year) 
    : err(false), dt_tm(0), stm_set (false), time_t_set (false)
  {
    struct tm s;
    s.tm_year = year - 1900;
    s.tm_mon = month - 1;
    s.tm_mday = day;
    s.tm_hour = 0;
    s.tm_min = 0;
    s.tm_sec = 0;
    set(s);
  }
  okdate_t (int month, int day, int year, int hour, int min, int sec) 
    : err(false), dt_tm(0), stm_set(false), time_t_set (false)
  {
    struct tm s;
    s.tm_year = year - 1900;
    s.tm_mon = month - 1;
    s.tm_mday = day;
    s.tm_hour = hour;
    s.tm_min = min;
    s.tm_sec = sec;
    set(s);
  }
  okdate_t () : 
    sec (0), min (0), hour (0), err (false), dt_tm (0), stm_set (false),
    time_t_set (false) {}

  template<class C> static ptr<okdate_t> alloc (C x)
  { return New refcounted<okdate_t> (x); }

  void set (const x_okdate_t &x);
  void set (const x_okdate_time_t &x);
  void set (const x_okdate_date_t &x);
  void set (const struct tm &s);
  void set (time_t t);
  void set (const str &s);

  operator time_t () const { return to_time_t (); }

  virtual str to_str () const;
  void to_xdr (x_okdate_t *x) const;
  void to_xdr (x_okdate_date_t *x) const;
  void to_xdr (x_okdate_time_t *x) const;
  time_t to_time_t () const;
  void to_stm (struct tm *stmp) const;
  void set_stm () const;
  void to_strbuf (strbuf *b, bool quotes) const;

  bool valid() const;
		
  int sec;       // seconds (0 - 59)
  int min;       // minutes (0 - 59)
  int hour;      // hours (0 - 23)
  int mday;      // day of month (1 - 31)
  int mon;       // month of the year (1 - 12)
  int year;      // year (XXXX)

  bool err;
  mutable struct tm stm; // used for date arithmetic
  u_int dt_tm;
  mutable bool stm_set;  // if the above is set;
  mutable bool time_t_set;
  mutable time_t time_t_val;

private:
  str strip_zero(const str& s) const;
		
};

typedef ptr<okdate_t> okdatep_t;

sex_t str_to_sex (const str &s);
sex_t str_to_sex (const char *c, u_int len);
str sex_to_str (sex_t t);
char sex_to_char (sex_t t);
sex_t char_to_sex (char c);

#endif /* _LIBWEB_WEB_H */
