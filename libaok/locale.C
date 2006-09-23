
#include "oklocale.h"
#include "okcgi.h"

namespace std_locale {

  void
  locale_t::insert (strbuf &b, insert_type_t t, 
		    const char *s1, const char *s2) const
  {
    if ((int (t) & int (INSERT_LANG)) && _lang) {
      if (s1) b << s1;
      b << _lang;
      if (s2) b << s2;
    }

    if ((int (t) & int (INSERT_PARTITION)) && _partition) {
      if (s1) b << s1;
      b << _partition;
      if (s2) b << s2;
    }
  }
  
  str
  localizer_t::localize (const str &infn) const
  {
    static rxx fn_rxx ("(.*/)?([^./]*)(\\.[^/]*)");
    if (!fn_rxx.match (infn)) {
      return infn;
    } else {
      strbuf b;
      if (fn_rxx[1]) {
	b << fn_rxx[1];
      }
      _locale->insert (b, _dir, NULL, "/");
      if (fn_rxx[2]) {
	b << fn_rxx[2];
      }
      _locale->insert (b, _sffx1, ".", NULL);
      if (fn_rxx[3]) {
	b << fn_rxx[3];
      }
      _locale->insert (b, _sffx2, ".", NULL);
      return b;
    }
  }

  localizer_factory_t::localizer_factory_t (const str &cfg)
  {
    parse (cfg);
  }

  static insert_type_t str2type (const str &in)
  {
    int ret = 0;
    if (in) {
      for (const char *cp = in.cstr (); *cp; cp++) {
	if (*cp == 'l' || *cp == 'L') {
	  ret |= int (INSERT_LANG);
	} else if (*cp == 'p' || *cp == 'P') {
	  ret |= int (INSERT_PARTITION);
	}
      }
    }
    return insert_type_t (ret);
  }

  void
  localizer_factory_t::parse (const str &cfg)
  {
    str d, s1, s2;
    ptr<cgi_t> t (cgi_t::str_parse (cfg));
    t->lookup ("dir", &d);
    t->lookup ("sffx1", &s1);
    t->lookup ("sffx2", &s2);
    _dir = str2type (d);
    _sffx1 = str2type (s1);
    _sffx2 = str2type (s2);
  }

  ptr<localizer_t>
  localizer_factory_t::mk_localizer (ptr<const locale_t> in) const
  {
    return New refcounted<localizer_t> (_dir, _sffx1, _sffx2, in);

  }
}

