// license:BSD-3-Clause

/***************************************************************************
 * PinMAME Remote Debugger - minimal embedded HTTP server                  *
 *                                                                         *
 * Single-threaded HTTP/1.1 server running in its own pthread. Regular     *
 * requests are served strictly one at a time; in addition a small number  *
 * of Server-Sent-Events (SSE) clients can stay connected and receive      *
 * push notifications polled from an event source callback.                *
 *                                                                         *
 * Threading: the handler callback runs on the server thread. It must do   *
 * its own locking against the emulator thread (see remote_debug_lock()).  *
 ***************************************************************************/
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

/* Parsed HTTP request. query is the part after '?' (may be empty). */
typedef struct http_request {
	char method[16];
	char path[256];
	char query[512];
} http_request_t;

/* Response filled in by the handler.
 * body must be malloc()ed (the server frees it) or NULL.
 * status is an HTTP status code (200, 400, 404, 500, ...).
 * Setting sse requests an SSE upgrade: the server sends a text/event-stream
 * header and keeps the connection open; body/len are ignored in that case. */
typedef struct http_response {
	char *body;
	int   len;
	int   status;
	int   sse;
	char  content_type[64];
} http_response_t;

/* Request handler; runs on the HTTP server thread. */
typedef void (*http_handler_t)(const http_request_t *req, http_response_t *resp);

/* Event source for SSE push. Must copy the next pending event (a complete
 * JSON object) into buf (NUL-terminated, at most maxlen bytes including the
 * terminator) and return 1, or return 0 if no event is pending.
 * Runs on the HTTP server thread. */
typedef int (*http_event_source_t)(char *buf, int maxlen);

/* Start the server thread listening on port. events may be NULL if SSE
 * push is not used. Returns 0 on success, -1 on error. */
int http_server_start(int port, http_handler_t handler, http_event_source_t events);

/* Stop the server thread and wait for it to terminate. After this returns
 * no handler is running anymore. */
void http_server_stop(void);

#endif /* HTTP_SERVER_H */
