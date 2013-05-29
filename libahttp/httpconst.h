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

#define HTTP_OK                  200
#define HTTP_CREATED             201
#define HTTP_ACCEPTED            202
#define HTTP_PARTIAL_CONTENT     206

#define HTTP_MOVEDPERM           301
#define HTTP_MOVEDTEMP           302
#define HTTP_SEEOTHER            303
#define HTTP_NOT_MODIFIED        304
#define HTTP_USEPROXY            305

#define HTTP_BAD_REQUEST         400
#define HTTP_UNAUTHORIZED        401
#define HTTP_FORBIDDEN           403
#define HTTP_NOT_FOUND           404
#define HTTP_NOT_ALLOWED         405
#define HTTP_NOT_ACCEPTABLE      406
#define HTTP_TIMEOUT             408
#define HTTP_GONE                410
#define HTTP_REQ_TOO_BIG         413
#define HTTP_URI_TOO_BIG         414
#define HTTP_UNSUPPORTED_MEDIA   415
#define HTTP_UNEXPECTED_EOF      417

#define HTTP_SRV_ERROR           500
#define HTTP_NOT_IMPLEMENTED     501
#define HTTP_BAD_GATEWAY         502
#define HTTP_UNAVAILABLE         503


#define HTTP_REDIRECT    HTTP_MOVEDPERM

//
// these are not in the standard, but are here for convenience.
//
#define HTTP_CLIENT_EOF  600
#define HTTP_CONNECTION_FAILED 601
#define HTTP_CLIENT_BAD_PROXY 602
#define HTTP_SERVER_SHUTDOWN 603
#define HTTP_DNS_FAILED 604
#define HTTP_PIPELINE_EOF 605
#define HTTP_PIPELINE_CLEAN_TIMEOUT 606
#define HTTP_NO_STATUS 0

#define HTTP_CRLF "\r\n"

typedef enum {
  RFC_1036  = 1036,
  RFC_1123  = 1123
} rfc_number_t;

const char *rfc_date_fmt (rfc_number_t rfc);

namespace httpconst {
  bool is_redirect (int status);
  bool is_error (int status);
};

#endif /* _LIBAHTTP_HTTPCONST */
