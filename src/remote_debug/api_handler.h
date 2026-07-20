// license:BSD-3-Clause

/***************************************************************************
 * PinMAME Remote Debugger - HTTP API dispatch                             *
 *                                                                         *
 * Maps request paths to handler functions; see /api/doc (or README.md)    *
 * for the endpoint reference. Runs on the HTTP server thread.             *
 ***************************************************************************/
#ifndef API_HANDLER_H
#define API_HANDLER_H

#include "http_server.h"

/* Handler for one API route. */
typedef void (*api_route_fn)(const http_request_t *req, http_response_t *resp);

typedef struct {
	const char *path;     /* exact request path */
	api_route_fn handler;
	const char *doc;      /* one-line description incl. parameters */
} api_route_t;

/* Top level request handler passed to http_server_start(). */
void api_handler(const http_request_t *req, http_response_t *resp);

#endif /* API_HANDLER_H */
