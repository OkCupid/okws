
# OKWS Release News

**1 Dec 2011**

  * Move everything over to GitHub for primary project hosting!

**7 Jun 2011**: OKWS v3.1.13.2

  * Fix a bug in the ''fork()'' pub filter, having to do with race conditions in the environment.

**3 Jun 2011**: OKWS v3.1.13.1

  * Fix a bug in the ''len()'' pub filter -- make it UTF-8 aware.

**26 May 2011**: OKWS v3.1.13

  * Bug fixes in sorting with negative floats
  * ''sort2()'' that takes a keyfunc, rather than a comparator.  Should be more efficient
  * Implement python-style recursive list-comparison. **Beware** that it's not resilient to infinite recursion.  Need to fix that eventually.

**24 May 2011**: OKWS v3.1.12

  * ''libaok.so'' no longer depends on the ''okrfn3'' internals.
  * a patterned interface for pub filters that use the blocking interface, like the one we had for filters that use the non-blocking interface. 

**20 May 2011**: OKWS v3.1.11

  * New libamysql feature  -- ''mysql_var_t'', which when passed to ''execute'', is escaped as `foo`, rather than 'foo'.  This allows for statements of the form "SELECT ? FROM ? WHERE ? = ?".  One would execute that statement with ''->execute (mysql_var_t ("a"), mysql_var_t ("b"), mysql_var_t ("c"), d)''.
  * New library functions: ''log'', ''exp'', ''pow'', ''sqrt'', ''bitwise_or'', ''bitwise_and'', ''splice'', ''shuffle'', ''randsel''.
  * Bug workaround: if connecting to logger fails on startup, can cause a service to quit, and then try again.  Enable with configuration option ''DieOnLogdCrash.''

**25 Apr 2011**: OKWS v3.1.9.1

  * Fix some bugs and ugly code having to do with Gzip and content chunking.  Separate the two ideas better (but sadly not all of the way). Disable chunking for older versions of Safari, which backs out an earlier hack that was breaking flat file service through the Akamai proxy.

**21 Apr 2011**: OKWS v3.1.8

  * A new pub filter called fork(), that allows "fire-and-forget" semantics for blocking pub filters.  To make this work, changed the signatures of the pub functions to take a ''ptr<eval_t>'' rather than a ''eval_t *'', and both sides of the fork (the foreground and background) hold a reference to the evaluation state.

**27 Mar 2011**: OKWS v3.1.7

  * A ''raw'' filter that reads raw pub files into a pub string.
  * An ''exit'' statement that allows short-circuiting out of an include file. I.e., ''exit'' is to a  file as ''return'' is to a function.

**22 Mar 2011**: OKWS v3.1.6.2

  * The ''replace'' pub function has been spruced up: the replacement pattern can now either contain capture groups; alternatively, the replacement pattern can be a lambda as in Python's ''re.sub''.

**17 Mar 2011**: OKWS v3.1.6

  * SSLCipherList support

**Feb 2011:** OKWS v3.1.5

  * Runtime pub profiler to figure out where your CPU cycles in pub are spent (documentation needed)
  * HTTP-keepalive now fully supported.
  * Allow fully-customizable logger support (e.g., to Facebook scribe).  Previously, only ''logger'' was supported.
  * Various bugfixes:
    * Naive-gzip fixed (though still I recommend using "smart", which is on by default)
    * HTTPS client code now behaves more rationally when you ask it to connect to a non-SSL port
    * Fix various FD-inheritance issues with services binding directly to ports.

**Jan 2011:** OKWS 3.1.4

   * Automatically kill unresponsive services
   * ''xdropq2json'' and ''json2xdropq'' --- convert an XDR opaque buffer into JSON, as long as you tell it which XDR type it is.  Go in reverse, too.

**Jan 2011:** SVN repository moved to ''svn3.okws.org''

**Summer 2010:** OKWS v3.1 released!
