// -*-c++-*-
//
// Class for handling e-mail addresses
//
// $Id$

//-----------------------------------------------------------------------------

#include "email.h"

#include "okerr.h"

//-----------------------------------------------------------------------------
/*
rxx email::valid_pattern("^(([a-zA-Z0-9_\\.\\-])+)\\@((([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+)$", "S");
*/
//-----------------------------------------------------------------------------

email::email(const str& s) :
	username(""),
	hostname(""),
	valid(false)
{
	rxx valid_pattern("^(([a-zA-Z0-9_\\.\\-])+)\\@((([a-zA-Z0-9\\-])+\\.)+([a-zA-Z0-9]{2,4})+)$", "S");

	if (s) {
		valid = valid_pattern.match(s);
	}

	if (valid) {
		if (valid_pattern[1]) {
			username = valid_pattern[1];
		}
		if (valid_pattern[3]) {
			hostname = valid_pattern[3];
		}
	}
}

//-----------------------------------------------------------------------------

bool
email::isValid() const
{
	return valid;
}

//-----------------------------------------------------------------------------

str
email::getUsername() const
{
	return username;
}

//-----------------------------------------------------------------------------

str
email::getHostname() const
{
	return hostname;
}

//-----------------------------------------------------------------------------
