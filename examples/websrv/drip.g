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

class oksrvc_drip_t : public oksrvc_t {
public:
  oksrvc_drip_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
};

class okclnt_drip_t : public okclnt_t {
public:
  okclnt_drip_t (ptr<ahttpcon> x, oksrvc_drip_t *o) 
    : okclnt_t (x, o), ok_drip (o) {}
  ~okclnt_drip_t () {}

  void schedule_drip () 
  {
    delaycb (1, 0, wrap (this, &okclnt_drip_t::drip));
  }


  void drip ()
  {
    bool done = false;
    strbuf b;
    /*o
      print (b) <<EOF;
Drip...Drip...Drip....@{i}<br>
EOF
     o*/

    if (i++ == 10) {
      /*o
	print (b) <<EOF;
All done!<br>
 </body>
</html>
EOF
      o*/
      done = true;
    }

    if (!output_fragment (b) || done)
      output_done ();
    else
      schedule_drip ();
  }

  void process ()
  {
    strbuf b;
    /*o
        print (b) <<EOF;
<html>
 <head>
  <title>OKWS Drip Test</title>
 </head>
<body>
Starting the drip (will go from 1 to 10)<br>
EOF
    o*/
    output_hdr ();
    if (!output_fragment (b)) 
      output_done ();
    else {
      i = 1;
      schedule_drip ();
    }
  }
  oksrvc_drip_t *ok_drip;
  int i;
};

okclnt_t *
oksrvc_drip_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_drip_t (x, this); 
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_drip_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
