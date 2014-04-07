// -*-c++-*-
/* $Id$ */

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

#ifndef _LIBPUB_OKWS_SFS_H
#define _LIBPUB_OKWS_SFS_H 1

/* annoying, need to always do this first */
#ifndef __STDC_LIMIT_MACROS
# define __STDC_LIMIT_MACROS 1
#endif
#include <stdint.h>


#include "async.h"
#include "amisc.h"
#include "refcnt.h"
#include "str.h"

#if !defined(SFSLITE_AT_VERSION) 
# error "Need sfslite patchlevel of 1.2.9.13 greater!"
#else
# if !SFSLITE_AT_VERSION(1,2,9,113)
#  error "Need sfslite patchlevel of 1.2.9.13 greater!"
# endif
#endif


// Patch Level <major>.<minor>.<pre> (out to 2 places)
// patch levels 0-99 are preleases.
// patch level 100 is release
#define OKWS_VERSION_MAJOR 3
#define OKWS_VERSION_MINOR 1
#define OKWS_VERSION_PATCHLEVEL 20
#define OKWS_VERSION_PRE 100

#define OKWS_AT_VERSION(Maj,Min,Pat,Pre) \
  (VERSION_FLATTEN(Maj,Min,Pat,Pre) <= \
   VERSION_FLATTEN(OKWS_VERSION_MAJOR, \
		   OKWS_VERSION_MINOR, \
                   OKWS_VERSION_PATCHLEVEL, \
                   OKWS_VERSION_PRE))

#define OKWS_PATCHLEVEL_STR OKWS_VERSION

//
// A header file for OKWS changes to SFS. Should be included by most files
// in OKWS and also anything build with OKWS.
//

TYPE2STRUCT(, long long);
TYPE2STRUCT(, short);
TYPE2STRUCT(, unsigned short);
TYPE2STRUCT(, unsigned long long);

#define sNULL static_cast<str> (NULL)

#ifndef HAVE_SFS_CLOCK_T
# ifndef HAVE_SFS_SET_CLOCK
typedef enum { SFS_CLOCK_GETTIME = 0, 
	       SFS_CLOCK_MMAP = 1, 
	       SFS_CLOCK_TIMER = 2 } sfs_clock_t;
# endif
#endif

#include "tame.h"

//
// Ideally, we could use cb->signal() freely within the code, but it's
// nice to still be compatible with existing SFS.
//
#ifndef TRIGGER
# define TRIGGER (cb, ...) (*cb)(__VA_ARGS__)
#endif

#endif /* LIBPUB_OKWS_SFS_H */

