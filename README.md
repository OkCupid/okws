# OKWS  -- The OK Web Server

OKWS is a Web server, specialized for building fast and secure Web
services. It provides Web developers with a small set of tools that
has proved powerful enough to build complex systems with limited
effort. Despite its emphasis on security, OKWS shows performance
advantages relative to popular competitors. Commercial experience with
OKWS suggests that the system can reduce hardware and system
management costs, while providing security guarantees absent in
current systems.

As of 8 Dec 2011, OKWS is still being maintained and worked on, with
3.1 being the active development version.  There are no big
feature-development plans on the horizon, but we're regularly
committing bugfixes and extensions.

## News

**1 Dec 2011**

  * Move everything over to GitHub for primary project hosting!

See our [News](/okws/okws/blob/master/NEWS.md) file for a fuller list of updates.

## A Better Architecture

The basic idea behind OKWS is that each Web service you write (such as
''search'' or ''newsletter-subscribe'') should run as a single
process. There is no reason to sprinkle the same program across
hundreds of address spaces just to get client concurrency. OKWS is
built with this principle in mind. A Web service in OKWS is compiled
into a free-standing process, calling upon our boilerplate
libraries. Other standard helper processes run on the system, to
direct traffic, to launch and relaunch applications should they crash,
to log HTTP transactions to disk, and to access static HTML templates.

Programming in OKWS follows the single-threaded, event-driven
model. No need to worry about the synchronization snafus that creep up
when using multi-threaded or multi-process Web servers.

## Who Runs OKWS?

Some example sites include:

* [OkCupid: Free Online Dating](http://www.okcupid.com)
* [MovieMadness](http://www.movie-madness.org)
* [Addgene Plasmid Repository](http://www.addgene.com)

## Platform Support

OKWS runs on most Unixes, and has been tested extensively on FreeBSD and Linux.

## Software Tools

Though OKWS is event-based at its core, A new tool, that is entirely
compatible with existing libasync-based code, is [[sfslite::tame2]],
which we urge you to read more about.  It's a simple C++-level
rewriter that makes your event code look more like threaded coded.
I.e., you can make a sequence of serial blocking functions within one
function body.

## Download

OKWS is freely available under a GPL v2 license.

## Academic Publications

* _Building Secure High-Performance Web Services With OKWS_.
  Maxwell Krohn,
  in _Proceedings of the 2004 USENIX Annual Technical Conference_ 
  [USENIX 2004](http://www.usenix.org/events/usenix04/),
  Boston, MA, June 2004. 
  [ [ps](http://pdos.lcs.mit.edu/~max/docs/okws.ps)  |
    [ps.gz](http://pdos.lcs.mit.edu/~max/docs/okws.ps.gz) |
    [pdf](http://pdos.lcs.mit.edu/~max/docs/okws.pdf) ].

* _Events Can Make Sense_.
  Max Krohn, Eddie Kohler and M. Frans Kaashoek, 
  in _Proceedings of the 2007 USENIX Annual Technical Conference_
  [USENIX 2007](http://www.usenix.org/events/usenix07/),
  Santa Clara, CA, June 2007. 
  [ [ps](http://pdos.lcs.mit.edu/~max/docs/tame.ps) | 
    [ps.gz](http://pdos.lcs.mit.edu/~max/docs/tame.ps.gz) | 
    [pdf](http://pdos.lcs.mit.edu/~max/docs/tame.pdf) ].

## Our GitHub Wiki Documentation Center

  * [Pub: OKWS's Publishing Language](https://github.com/okws/okws/wiki/pub)
  * [Install Instructions](https://github.com/okws/okws/wiki/install)
  * [Wiki Table of Contents](https://github.com/okws/okws/wiki)

## Active People

  * Mike Maxim
  * David Koh
  * Tom Jacques
  * Till Varoquaux

## Patch Contributors

  * Ben Hollenstein
  * Ian Rickard (Apple Darwin Patches)
  * Michael Walfish (various patches and fixes for API problems).
  * Benjie Chen
  * OkCupid team, including Tom Quisel, Mike Maxim, Tom Jacques, and Eli Gwynn.

