
// -*-c++-*-
/* $Id$ */

#ifndef _LIBAHTTP_HTTPCONST
#define _LIBAHTTP_HTTPCONST

#define HTTP_OK          200
#define HTTP_PARTIAL_CONTENT 206
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

#define OKD_SERVER_ID "OKD/"VERSION

#define HTTP_CRLF "\r\n"

#endif /* _LIBAHTTP_HTTPCONST */
