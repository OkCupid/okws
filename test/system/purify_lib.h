
// -*-c++-*-
/* $Id: purify.T 3190 2008-02-05 15:10:03Z max $ */

/*
 *
 * Copyright (C) 2003-4 by Maxwell Krohn (max@okcupid.com)
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

#ifndef _LIBHTML_PURIFY_H_
#define _LIBHTML_PURIFY_H_

#include "ok.h"
#include "okcgi.h"
#include "pub.h"
#include <unistd.h>
#include "tame.h"
#include "ok_adebug.h"
#include "test_const.h"
#include "okwcxml.h"


namespace purify {
       
  typedef enum { OK = 0, 
		 ERR_INIT = 1,
		 ERR_TIMEOUT = 2, 
		 ERR_XML = 3, 
		 ERR_PURIFY = 4 } status_t;

  extern const char *conf_file_key;
  
  typedef event<status_t, str>::ref ev_t;
  
  class purifier_t {
  public:
    
    purifier_t () : _timeout (10) {}
    void init (pub3::ok_iface_t *pub,
	       str file, evb_t ev, str key = NULL, CLOSURE);
    bool init (const str &s);
    void purify (str in, ev_t ev, CLOSURE);
    void set_timeout (int t) { _timeout = t; }
    
  private:
    bool init ();
    
    ptr<okwc3::agent_xml_t> _agent;
    int _timeout;
    str _url;
  };
};


#endif /* _LIBHTML_PURIFY_H_ */
