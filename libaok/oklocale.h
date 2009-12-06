// -*-c++-*-
/* $Id: pub.h 2186 2006-09-19 13:25:16Z max $ */

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

#ifndef _LIBAOK_OKLOCALE_H_
#define _LIBAOK_OKLOCALE_H_ 1

#include "pub.h"
#include "async.h"

//
// The standard locale engine assumes that each user session
// supplies 2 parameters, a LANG and a PARTITION, signifying
// a language a site partition. 
//
namespace std_locale {
  
  typedef enum {
    INSERT_NONE = 0,
    INSERT_LANG = 0x1,
    INSERT_PARTITION = 0x2
  } insert_type_t;

  class locale_t {
  public:
    locale_t (const str &l, const str &p = NULL) 
      : _lang (l), _partition (p) {}
    str lang () const { return _lang; }
    str partition () const { return _partition; }

    static ptr<locale_t> alloc (const str &l, const str &p = NULL)
    { return New refcounted<locale_t> (l, p); }
    
    void insert (strbuf &b, insert_type_t t,
		 const char *s1, const char *s2) const;
  private:
    const str _lang;
    const str _partition;
  };
  
  class localizer_t : public pub3::localizer_t {
  public:
    localizer_t (insert_type_t d, insert_type_t s1, insert_type_t s2,
		 ptr<const locale_t> l)
      : pub3::localizer_t (), _dir (d), _sffx1 (s1), _sffx2 (s2), _locale (l) {}
    
    virtual str localize (const str &infn) const;
    
  private:
    insert_type_t _dir, _sffx1, _sffx2;
    ptr<const locale_t> _locale;
  };
  
  class localizer_factory_t {
  public:
    localizer_factory_t (const str &cfg);
    static ptr<localizer_factory_t> alloc (const str &cfg)
    { return New refcounted<localizer_factory_t> (cfg); }
    void parse (const str &cfg);
    ptr<localizer_t> mk_localizer (ptr<const locale_t> l) const;
    ptr<localizer_t> mk_localizer (const str &l, const str &p = NULL) const
    { return mk_localizer (locale_t::alloc (l, p)); }
  private:
    insert_type_t _dir, _sffx1, _sffx2;
  };
};

#endif /* _LIBAOK_OKLOCALE_H_ */
