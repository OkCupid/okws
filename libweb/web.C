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
#include "okconst.h"
#include "web.h"
#include "rxx.h"
#include "parseopt.h"

sex_t
str_to_sex (const str &s)
{
  if (!s)
    return NOSEX; // ; i don't need it as long as i have my computer
  return str_to_sex (s.cstr (), s.len ());
}

sex_t
str_to_sex (const char *s, u_int len)
{
  if (len < 1) 
    return NOSEX;
  if (*s == 'M' || *s == 'm')
    return MALE;
  if (*s == 'F' || *s == 'f')
    return FEMALE;
  return NOSEX;
}

str
sex_to_str (sex_t t) 
{
  switch (t) {
  case MALE:
    return "m";
  case FEMALE:
    return "f";
  default:
    return "";
  }
}

char
sex_to_char (sex_t t)
{
  switch (t) {
  case MALE:
    return 'm';
  case FEMALE:
    return 'f';
  default:
    return 'n';
  }
}

sex_t
char_to_sex (char c)
{
  switch (c) {
  case 'm':
  case 'M':
    return MALE;
  case 'f':
  case 'F':
    return FEMALE;
  default:
    return NOSEX;
  }
}

const char *date_rxx_str = "(([0-9]{4})-([0-9]{2})-([0-9]{2}))?" 
  "( ([0-9]{2}):([0-9]{2}):([0-9]{2}))?";

static rxx g_date_rxx (date_rxx_str);

//
// this is needed so convertint doesn't think strings like "09" are octal
//
static str strip_zero(const str& s)
{
  const char *cp = s.cstr ();
  while (*cp == '0')
    cp ++;

  // If we got '00000' then make it '0'
  if (*cp == '\0' && s.len () > 0) 
    cp --;

  return cp;
}

void
okdate_t::clear_c_times ()
{
  time_t_set = false;
  stm_set = false;
}

void
okdate_t::clear ()
{
  sec = min = hour = mday = mon = year = 0;
  err = false;
  memset (&stm, 0, sizeof (stm));
  dt_tm = 0;
  clear_c_times ();
}

void
okdate_t::set (const str &s)
{
  rxx *x = NULL;
  rxx *new_rxx = NULL;

  if (ok_kthread_safe) {
    x = new_rxx = New rxx (date_rxx_str);
  } else {
    x = &g_date_rxx;
  }
    
  if (!s || !x->match (s)) {
    err = true;
  } else {

    err = false;
  
    if ((*x)[1]) {
      dt_tm |= OK_DATE;
    
      bool ok = convertint (strip_zero((*x)[2]), &year) &&
	convertint (strip_zero((*x)[3]), &mon) &&
	convertint (strip_zero((*x)[4]), &mday);
      assert (ok);

    }
    
    if ((*x)[5]) {
      dt_tm |= OK_TIME;
      
      bool ok = convertint (strip_zero((*x)[6]), &hour) &&
	convertint (strip_zero((*x)[7]), &min) &&
	convertint (strip_zero((*x)[8]), &sec);
      assert (ok);
    }
  }
  clear_c_times ();

  if (new_rxx) delete new_rxx;
}

bool
okdate_t::set (const str &s, long gmt_off)
{
  rxx *x = NULL;
  rxx *new_rxx = NULL;

  if (ok_kthread_safe) {
    x = new_rxx = New rxx (date_rxx_str);
  } else {
    x = &g_date_rxx;
  }
    
  if (!s || !x->match (s)) { 
    err = true;
  } else {
    struct tm t;
    memset (&t, 0, sizeof (t));
    bool time = false;
    err = false;

    if ((*x)[1]) {
      bool ok = 
	convertint (strip_zero((*x)[2]), &t.tm_year) &&
	convertint (strip_zero((*x)[3]), &t.tm_mon) &&
	convertint (strip_zero((*x)[4]), &t.tm_mday);
      assert (ok);
    }
    
    if ((*x)[5]) {
      time = true;
      bool ok = 
	convertint (strip_zero((*x)[6]), &t.tm_hour) &&
	convertint (strip_zero((*x)[7]), &t.tm_min) &&
	convertint (strip_zero((*x)[8]), &t.tm_sec);
      assert (ok);
    }

    if (time) {
      t.tm_year -= 1900;
      t.tm_mon -= 1;
      set (t, gmt_off);
    } else {
      year = t.tm_year;
      mon = t.tm_mon;
      mday = t.tm_mday;
      dt_tm |= OK_DATE;
    }
  }
  clear_c_times ();

  if (new_rxx) delete new_rxx;
  return !err;
}

static time_t
to_utc (const struct tm &t, long gmt_offset)
{
  time_t ret;
  struct tm tmp = t;

#ifdef STRUCT_TM_GMTOFF
  tmp.STRUCT_TM_GMTOFF = gmt_offset;
  ret = mktime (&tmp);
  
#else /* ! STRUCT_TM_GMTOFF */
  ret = mktime (&tmp);
  ret += gmt_offset;
#endif

  return ret;
}

void
okdate_t::set (const struct tm &s, long gmt_off)
{
  set (to_utc (s, gmt_off));
}

void
okdate_t::set (time_t t)
{
  clear ();
  time_t_val = t;
  time_t_set = true;
  struct tm *stm = gmtime (&t);
  if (!stm)
    err = true;
  else {
    set (*stm);
    err = false;
  }
}

void
okdate_t::set (const struct tm &s)
{
  year = s.tm_year + 1900;
  mon = s.tm_mon + 1;
  mday = s.tm_mday;
  hour = s.tm_hour;
  min = s.tm_min;
  sec = s.tm_sec;
  stm = s;
  stm_set = true;
  time_t_set = false;
  dt_tm |= (OK_DATE | OK_TIME);
}

void
okdate_t::set (const x_okdate_date_t &x, long gmt_off)
{
  mday = x.mday;
  mon = x.mon;
  year = x.year;
  dt_tm |= OK_DATE;
  clear_c_times ();
  if (gmt_off)
    apply_gmt_off (gmt_off);
}

void
okdate_t::apply_gmt_off (long gmt_off)
{
  struct tm t;
  memset (&t, 0, sizeof (t));
  to_stm (&t);
  set (t, gmt_off);
}

void
okdate_t::set (const x_okdate_time_t &x, long gmt_off_dummy)
{
  sec = x.sec;
  min = x.min;
  hour = x.hour;
  clear_c_times ();
  dt_tm |= OK_TIME;
}

void
okdate_t::set (const x_okdate_t &x, long gmt_off)
{
  clear_c_times ();
  if (x.time.on) {
    set (*x.time.time);
  } else {
    sec = min = hour = 0;
  }
  if (x.date.on) 
    set (*x.date.date, gmt_off);
}

void
okdate_t::to_strbuf (strbuf *b, bool quotes, long gmt_off) const
{
  okdate_t tmp (to_time_t () + gmt_off);
  tmp.to_strbuf (b, quotes);
}

void
okdate_t::to_strbuf (strbuf *b, bool quotes) const
{
  if (quotes)
    (*b) << "\'";
  if (dt_tm & OK_DATE) 
    b->fmt ("%04d-%02d-%02d", year, mon, mday);
  if (dt_tm & (OK_DATE | OK_TIME))
    (*b) << " ";
  if (dt_tm & OK_TIME)
    b->fmt ("%02d:%02d:%02d", hour, min, sec);
  if (quotes)
    (*b) << "\'";
}

str
okdate_t::to_str (long gmt_off) const
{
  strbuf d;
  to_strbuf (&d, false, gmt_off);
  return d;
}

void
okdate_t::to_xdr (x_okdate_time_t *x) const
{
  x->hour = hour;
  x->min = min;
  x->sec = sec;
}

void
okdate_t::to_xdr (x_okdate_date_t *x) const
{
  x->year = year;
  x->mon = mon;
  x->mday = mday;
}

void
okdate_t::to_xdr (x_okdate_t *x) const
{
  if (dt_tm & OK_DATE) {
    x->date.set_on (true);
    to_xdr (x->date.date);
  } else 
    x->date.set_on (false);

  if (dt_tm & OK_TIME) {
    x->time.set_on (true);
    to_xdr (x->time.time);
  } else 
    x->time.set_on (false);
}

void
okdate_t::to_stm (struct tm *stmp) const
{
  if (stm_set) {
    *stmp = stm;
  } else {
    stmp->tm_sec = sec;
    stmp->tm_min = min;
    stmp->tm_hour = hour;
    stmp->tm_mday = mday;
    stmp->tm_mon = mon - 1;
    stmp->tm_year = year - 1900;
  }
}

void
okdate_t::set_stm () const
{
  if (!stm_set) {
    to_stm (&stm);
    stm_set = true;
  }
}

time_t
okdate_t::to_time_t () const
{
  set_stm ();
  if (!time_t_set) {
    time_t_val = timegm (&stm);
    time_t_set = true;
  }
  return time_t_val;
}

bool
okdate_t::valid() const
{
  bool result = true;
  
  if (mon <= 0 || mon > 12) {
    result = false;
  }

  if (mday <= 0 || mday > 31) {
    result = false;
  }

  if (mon == 2 && mday > 29) {
    result = false;
  }
  
  if (mon == 4 || mon == 6 || mon == 9 || mon == 11) {
    if (mday > 30) {
      result = false;
    }
  }
  
  if (year <= 0 || year > 9999) {
    result = false;
  }

  // leap year
  //
  if (mon == 2 && mday == 29) {
    // if not divisible by 4, then not a leap year
    //
    if (year % 4 != 0) {
      result = false;
    }
    else if ((year % 100 == 0) && (year % 400 != 0)) {
      result = false;
    }
  }
  
  return result;
}
