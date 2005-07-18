// -*-c++-*-
/* $Id$ */

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

#include "ok.h"
#include "okcgi.h"
#include "pub.h"
#include <unistd.h>


class oksrvc_upload_t : public oksrvc_t {
public:
  oksrvc_upload_t (int argc, char *argv[]) : oksrvc_t (argc, argv) { }
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
};

class okclnt_upload_t : public okclnt_t {
public:
  okclnt_upload_t (ptr<ahttpcon> x, oksrvc_upload_t *o) 
      : okclnt_t (x, o), ok_upload (o) {}
  ~okclnt_upload_t () {}

  void process ()
  {
    cgi_files_t *f;
    char *names[] = { "f1", "f2" };
    for (int i = 0; i < 2; i++) {
      if (!cgi.flookup (names[i], &f)) {
	out << "Cannot find field: " << names[i] << "\n";
      } else {
	out << "variable: " << names[i] << "<br>\n";

	// now f is a vector (almost always size 1) 
	// whose elements are cgi_file_t elements (note no "s")
	while (f->size ()) {
	  out << "-------------------" << "<br>\n";
	  cgi_file_t file = f->pop_back ();
	  out << "filename: " << file.filename << "<br>\n"
	      << "type: " << (file.type ? file.type : str ("(none)"))
	      << "<br>\n"
	      << "dat: " << file.dat << "<br><br>\n\n";
	}
      }
    }
    output (out);
  }
  oksrvc_upload_t *ok_upload;
};

okclnt_t *
oksrvc_upload_t::make_newclnt (ptr<ahttpcon> x)
{ 
  okclnt_t *ret = New okclnt_upload_t (x, this); 
  ret->enable_file_upload ();
  return ret;
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_upload_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
