// -*-c++-*-
/* $Id$ */

#include "ok.h"
#include "cgi.h"
#include "pub.h"
#include <unistd.h>

class okc_example_t : public oksrvc_t {
	public:
		okc_example_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
		okclnt_t *make_newclnt (ptr<ahttpcon> x);
		void init_publist () { /*<pub> init_publist (); </pub>*/ }
};

class okclnt_example_t : public okclnt_t {
	public:
		okclnt_example_t (ptr<ahttpcon> x, okc_example_t *o) 
			: okclnt_t (x, o), ok_example (o) {}
		~okclnt_example_t () {}

		void process ()
		{
			/*<pub>
			  include(pub, out, "/home/u1/patrick/tmp/template.html");
			</pub>*/
			output (out);
		}
		okc_example_t *ok_example;
};

okclnt_t *
okc_example_t::make_newclnt (ptr<ahttpcon> x)
{ 
	return New okclnt_example_t (x, this); 
}

int
main (int argc, char *argv[])
{
	oksrvc_t *okc = New okc_example_t (argc, argv);
	okc->launch ();
	amain ();
}
