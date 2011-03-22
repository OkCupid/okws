// -*-c++-*-

// OKWS improvements on sfslite's RXX stuff

#pragma once

#include "rxx.h"

// Like split from sfs, but also include the matched parameter in the
// output list.
int split2 (vec<str> *out, rxx pat, str expr, 
	    size_t lim = (size_t) -1, bool emptylast = false);

typedef callback<str, const vec<str> *>::ref rxx_replace_cb_t;

// Call the replace function for each matched pattern.
str rxx_replace (str input, rxx pat, rxx_replace_cb_t cb);

str rxx_replace (str input, rxx pat, str repl);

// In the string input, replace all occurrences of pat with
// repl.  Unlike rxx_replace, use capture support!
str rxx_replace_2 (str input, rxx pat, str repl);

