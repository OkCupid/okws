// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
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


#ifndef _LIBAHTTP_HTTPCONST
#define _LIBAHTTP_HTTPCONST

#define HTTP_OK          200
#define HTTP_PARTIAL_CONTENT 206
#define HTTP_NOT_MODIFIED 304
#define HTTP_URI_TOO_BIG 414
#define HTTP_BAD_REQUEST 400
#define HTTP_NOT_FOUND   404
#define HTTP_NOT_ALLOWED 405
#define HTTP_TIMEOUT     408
#define HTTP_NOT_IMPLEMENTED 502
#define HTTP_UNAVAILABLE 503
#define HTTP_REQ_TOO_BIG 413
#define HTTP_UNEXPECTED_EOF 417

#define HTTP_MOVEDPERM   301
#define HTTP_REDIRECT    HTTP_MOVEDPERM
#define HTTP_MOVEDTEMP   302
#define HTTP_SEEOTHER    303
#define HTTP_USEPROXY    305
#define HTTP_SRV_ERROR   500

//
// these are not in the standard, but are here for convenience.
//
#define HTTP_CLIENT_EOF  600
#define HTTP_CONNECTION_FAILED 601

#define HTTP_CRLF "\r\n"

#endif /* _LIBAHTTP_HTTPCONST */
