
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

static rxx date_rxx ("(([0-9]{4})-([0-9]{2})-([0-9]{2}))?" 
		     "( ([0-9]{2}):([0-9]{2}):([0-9]{2}))?");
void
okdate_t::set (const str &s)
{
  if (!date_rxx.match (s)) {
    err = true;
    return;
  }
  err = false;

  if (date_rxx[1]) {
    dt_tm |= OK_DATE;

    assert (convertint (strip_zero(date_rxx[2]), &year) &&
			convertint (strip_zero(date_rxx[3]), &mon) &&
			convertint (strip_zero(date_rxx[4]), &mday));
  }

  if (date_rxx[5]) {
    dt_tm |= OK_TIME;

    assert (convertint (strip_zero(date_rxx[6]), &hour) &&
			convertint (strip_zero(date_rxx[7]), &min) &&
			convertint (strip_zero(date_rxx[8]), &sec));
  }
}

//
// this is needed so convertint doesn't think strings like "09" are octal
//
str
okdate_t::strip_zero(const str& s) const
{
  if (s[0] == '0') {
	return substr(s, 1, s.len() - 1);
  }

  return s;
}

void
okdate_t::set (time_t t)
{
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
  dt_tm |= (OK_DATE | OK_TIME);
}

void
okdate_t::set (const x_okdate_date_t &x)
{
  mday = x.mday;
  mon = x.mon;
  year = x.year;
  dt_tm |= OK_DATE;

}

void
okdate_t::set (const x_okdate_time_t &x)
{
  sec = x.sec;
  min = x.min;
  hour = x.hour;
  dt_tm = OK_TIME;
}

void
okdate_t::set (const x_okdate_t &x)
{
  if (x.date.on) 
    set (*x.date.date);
  if (x.time.on)
    set (*x.time.time);
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
okdate_t::to_str () const
{
  strbuf d;
  to_strbuf (&d, false);
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
  return timegm (&stm);
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
