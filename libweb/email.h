// -*-c++-*-
//
// Class for handling e-mail addresses
//
// $Id$

//-----------------------------------------------------------------------------

#include "rxx.h"

//-----------------------------------------------------------------------------

class email {

	private:
		str username;
		str hostname;
		bool valid;

//		static rxx valid_pattern;
		
	public:
		email(const str& s);

		bool isValid() const;
		str getUsername() const;
		str getHostname() const;
	
};

//-----------------------------------------------------------------------------
