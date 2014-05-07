
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

#ifndef _LIBPUB_OKDBG2_H
#define _LIBPUB_OKDBG2_H 1

#include "okdbg-int.h"


#define OKWS_DEBUG_PUB_BINDTAB_INSERT             (1 << 0)
#define OKWS_DEBUG_PUB_BINDTAB_ACCESS             (1 << 1)
#define OKWS_DEBUG_PUB_BINDTAB_INSERTS            (1 << 2)
#define OKWS_DEBUG_PUB_ERRORS                     (1 << 3)
#define OKWS_DEBUG_OKD_SHUTDOWN                   (1 << 4)
#define OKWS_DEBUG_OKD_STARTUP                    (1 << 5)
#define OKWS_DEBUG_OKD_NOISY_CONNECTIONS          (1 << 6)
#define OKWS_DEBUG_HLP_STATUS                     (1 << 7)
#define OKWS_DEBUG_SVC_ARGS                       (1 << 8)
#define OKWS_DEBUG_OKD_JAIL                       (1 << 9)
#define OKWS_DEBUG_OKLD_FD_PASSING                (1 << 10)
#define OKWS_DEBUG_PUB3_CACHE                     (1 << 11)
#define OKWS_DEBUG_PUB3_CHUNKS                    (1 << 12)
#define OKWS_DEBUG_STALL_SIGCONT                  (1 << 13)
#define OKWS_DEBUG_SVC_DATABASES                  (1 << 14)
#define OKWS_DEBUG_SVC_STARTUP                    (1 << 15)
#define OKWS_DEBUG_SSL_MEM                        (1 << 16)
#define OKWS_DEBUG_SSL_PROXY                      (1 << 17)
#define OKWS_DEBUG_SSL_INDATA                     (1 << 18)
#define OKWS_DEBUG_SSL_OUTDATA                    (1 << 19)
#define OKWS_DEBUG_PUB_PARSE                      (1 << 20)
#define OKWS_DEBUG_PUB_RFN3                       (1 << 21)
#define OKWS_DEBUG_OKD_KEEPALIVE                  (1 << 22)
#define OKWS_DEBUG_SVC_MPFD                       (1 << 23)
#define OKWS_DEBUG_OKD_CHILDREN                   (1 << 24)

#endif /* _LIBPUB_OKDEBUG_H */
