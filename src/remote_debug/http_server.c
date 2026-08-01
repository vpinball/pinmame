// license:BSD-3-Clause

/***************************************************************************
 * PinMAME Remote Debugger - minimal embedded HTTP server                  *
 * See http_server.h for the interface description.                       *
 ***************************************************************************/
#ifdef REMOTE_DEBUG

#include "http_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>

/* Maximum number of concurrently connected SSE clients. */
#define SSE_MAX_CLIENTS 8
/* Maximum size of an incoming request (headers included). */
#define REQUEST_BUF_SIZE 8192
/* select() timeout; also the maximum latency for SSE event delivery. */
#define POLL_TIMEOUT_USEC 200000

static int server_fd = -1;
static volatile int stop_requested = 0;
static pthread_t server_thread;
static int thread_started = 0;
static http_handler_t global_handler = NULL;
static http_event_source_t global_events = NULL;
static int sse_clients[SSE_MAX_CLIENTS];

static const char *status_text(int status)
{
	switch (status) {
		case 200: return "OK";
		case 400: return "Bad Request";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 500: return "Internal Server Error";
		case 503: return "Service Unavailable";
		default:  return "OK";
	}
}

/* Write the whole buffer, retrying on partial writes and EINTR.
 * MSG_NOSIGNAL avoids SIGPIPE when the client disconnected.
 * Returns 0 on success, -1 on error. */
static int write_all(int fd, const char *buf, int len)
{
	int done = 0;
	while (done < len) {
		ssize_t n = send(fd, buf + done, (size_t)(len - done), MSG_NOSIGNAL);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		done += (int)n;
	}
	return 0;
}

/* Read until the end of the request headers ("\r\n\r\n"), the buffer is
 * full, or a short timeout expires. Returns the number of bytes read. */
static int read_request(int fd, char *buf, int maxlen)
{
	int total = 0;
	while (total < maxlen - 1) {
		fd_set fds;
		struct timeval tv;
		ssize_t n;
		int ret;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec = 0;
		tv.tv_usec = 500000;
		ret = select(fd + 1, &fds, NULL, NULL, &tv);
		if (ret <= 0)
			break;
		n = read(fd, buf + total, (size_t)(maxlen - 1 - total));
		if (n <= 0)
			break;
		total += (int)n;
		buf[total] = 0;
		if (strstr(buf, "\r\n\r\n") != NULL)
			break;
	}
	buf[total] = 0;
	return total;
}

/* Parse the request line ("GET /path?query HTTP/1.1") into req.
 * Returns 0 on success, -1 on a malformed request. */
static int parse_request(const char *buf, http_request_t *req)
{
	const char *method_end, *path_start, *path_end, *query;
	size_t n;

	memset(req, 0, sizeof(*req));
	method_end = strchr(buf, ' ');
	if (!method_end)
		return -1;
	n = (size_t)(method_end - buf);
	if (n >= sizeof(req->method))
		n = sizeof(req->method) - 1;
	memcpy(req->method, buf, n);
	req->method[n] = 0;

	path_start = method_end + 1;
	path_end = strchr(path_start, ' ');
	if (!path_end)
		return -1;

	query = memchr(path_start, '?', (size_t)(path_end - path_start));
	if (query) {
		n = (size_t)(path_end - query - 1);
		if (n >= sizeof(req->query))
			n = sizeof(req->query) - 1;
		memcpy(req->query, query + 1, n);
		req->query[n] = 0;
		path_end = query;
	}
	n = (size_t)(path_end - path_start);
	if (n >= sizeof(req->path))
		n = sizeof(req->path) - 1;
	memcpy(req->path, path_start, n);
	req->path[n] = 0;
	return req->path[0] ? 0 : -1;
}

/* Register fd as an SSE client and send the stream header.
 * Returns 0 on success, -1 if no slot is free or the write failed. */
static int sse_add_client(int fd)
{
	static const char header[] =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/event-stream\r\n"
		"Cache-Control: no-cache\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Connection: keep-alive\r\n\r\n"
		"retry: 2000\n\n";
	int i;
	for (i = 0; i < SSE_MAX_CLIENTS; i++) {
		if (sse_clients[i] < 0) {
			if (write_all(fd, header, (int)sizeof(header) - 1) < 0)
				return -1;
			sse_clients[i] = fd;
			return 0;
		}
	}
	return -1;
}

static void sse_drop_client(int i)
{
	if (sse_clients[i] >= 0) {
		close(sse_clients[i]);
		sse_clients[i] = -1;
	}
}

/* Send one event (a JSON object) to all connected SSE clients. */
static void sse_broadcast(const char *event_json)
{
	char frame[1200];
	int len, i;
	len = snprintf(frame, sizeof(frame), "data: %s\n\n", event_json);
	if (len < 0 || len >= (int)sizeof(frame))
		return;
	for (i = 0; i < SSE_MAX_CLIENTS; i++) {
		if (sse_clients[i] >= 0 && write_all(sse_clients[i], frame, len) < 0)
			sse_drop_client(i);
	}
}

/* Serve a single (non-SSE) request on fd: parse, dispatch, respond. */
static void serve_request(int fd)
{
	char buf[REQUEST_BUF_SIZE];
	http_request_t req;
	http_response_t resp;
	char header[512];
	int header_len;

	if (read_request(fd, buf, sizeof(buf)) <= 0)
		return;

	memset(&resp, 0, sizeof(resp));
	resp.status = 200;
	strcpy(resp.content_type, "text/plain");

	if (parse_request(buf, &req) != 0) {
		resp.body = strdup("bad request");
		resp.len = resp.body ? (int)strlen(resp.body) : 0;
		resp.status = 400;
	}
	else if (strcmp(req.method, "GET") != 0) {
		resp.body = strdup("only GET is supported");
		resp.len = resp.body ? (int)strlen(resp.body) : 0;
		resp.status = 405;
	}
	else if (global_handler) {
		global_handler(&req, &resp);
		if (resp.sse) {
			if (sse_add_client(fd) == 0)
				return; /* keep the connection open, fd now owned by sse_clients */
			free(resp.body);
			resp.body = strdup("too many event stream clients");
			resp.len = resp.body ? (int)strlen(resp.body) : 0;
			resp.status = 503;
			resp.sse = 0;
		}
	}
	else {
		resp.body = strdup("no handler");
		resp.len = resp.body ? (int)strlen(resp.body) : 0;
		resp.status = 500;
	}

	header_len = snprintf(header, sizeof(header),
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %d\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Connection: close\r\n\r\n",
		resp.status, status_text(resp.status), resp.content_type, resp.len);
	if (header_len > 0 && header_len < (int)sizeof(header)
			&& write_all(fd, header, header_len) == 0
			&& resp.body && resp.len > 0)
		write_all(fd, resp.body, resp.len);
	free(resp.body);
}

static void *http_server_thread(void *arg)
{
	int port = *(int *)arg;
	struct sockaddr_in address;
	int opt = 1;
	int i;
	free(arg);

	for (i = 0; i < SSE_MAX_CLIENTS; i++)
		sse_clients[i] = -1;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("remote_debug: socket");
		return NULL;
	}
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		perror("remote_debug: setsockopt");

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons((unsigned short)port);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("remote_debug: bind");
		close(server_fd);
		server_fd = -1;
		return NULL;
	}
	if (listen(server_fd, 10) < 0) {
		perror("remote_debug: listen");
		close(server_fd);
		server_fd = -1;
		return NULL;
	}

	printf("Remote Debugger: HTTP server listening on port %d\n", port);

	while (!stop_requested) {
		fd_set fds;
		struct timeval tv;
		int maxfd = server_fd;
		int ret;

		FD_ZERO(&fds);
		FD_SET(server_fd, &fds);
		for (i = 0; i < SSE_MAX_CLIENTS; i++) {
			if (sse_clients[i] >= 0) {
				FD_SET(sse_clients[i], &fds);
				if (sse_clients[i] > maxfd)
					maxfd = sse_clients[i];
			}
		}
		tv.tv_sec = 0;
		tv.tv_usec = POLL_TIMEOUT_USEC;
		ret = select(maxfd + 1, &fds, NULL, NULL, &tv);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror("remote_debug: select");
			break;
		}

		/* Readable SSE clients have either disconnected or sent data we
		 * do not expect; drop them in both cases. */
		for (i = 0; i < SSE_MAX_CLIENTS; i++) {
			if (sse_clients[i] >= 0 && FD_ISSET(sse_clients[i], &fds))
				sse_drop_client(i);
		}

		/* Push pending events to SSE clients. */
		if (global_events) {
			char event[1024];
			while (global_events(event, (int)sizeof(event)))
				sse_broadcast(event);
		}

		if (FD_ISSET(server_fd, &fds)) {
			socklen_t addrlen = sizeof(address);
			int client = accept(server_fd, (struct sockaddr *)&address, &addrlen);
			if (client < 0) {
				if (errno != EINTR)
					perror("remote_debug: accept");
				continue;
			}
			serve_request(client);
			/* SSE upgrades keep the fd; everything else is closed here.
			 * Closing an fd twice would be a bug, so check ownership. */
			for (i = 0; i < SSE_MAX_CLIENTS; i++) {
				if (sse_clients[i] == client)
					break;
			}
			if (i == SSE_MAX_CLIENTS)
				close(client);
		}
	}

	for (i = 0; i < SSE_MAX_CLIENTS; i++)
		sse_drop_client(i);
	close(server_fd);
	server_fd = -1;
	return NULL;
}

int http_server_start(int port, http_handler_t handler, http_event_source_t events)
{
	int *port_ptr;
	global_handler = handler;
	global_events = events;
	stop_requested = 0;
	port_ptr = malloc(sizeof(int));
	if (!port_ptr)
		return -1;
	*port_ptr = port;
	if (pthread_create(&server_thread, NULL, http_server_thread, port_ptr) != 0) {
		perror("remote_debug: pthread_create");
		free(port_ptr);
		return -1;
	}
	thread_started = 1;
	return 0;
}

void http_server_stop(void)
{
	if (!thread_started)
		return;
	stop_requested = 1;
	pthread_join(server_thread, NULL);
	thread_started = 0;
}

#endif /* REMOTE_DEBUG */
