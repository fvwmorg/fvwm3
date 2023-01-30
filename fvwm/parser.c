/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <event2/event.h>
/* FIXME: including event_struct.h won't be binary comaptible with future
 * versions of libevent.
 */
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_compat.h>
#include <event2/listener.h>
#include <event2/util.h>

#include "parser.h"
#include "libs/getpwuid.h"
#include "libs/safemalloc.h"
#include "libs/queue.h"

#define FVWM_PARSER_SOCKET "fvwm_parser.sock"

static int debug = 1;
static char *sock_pathname;

struct client {
	struct bufferevent	*comms;

	TAILQ_ENTRY(client)	 entry;
};

TAILQ_HEAD(clients, client);
struct clients  clientq;

int
setnonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0)
		return -1;

	return 0;
}

static void
client_read_cb(struct bufferevent *bev, void *ctx)
{
	struct client	*c = ctx;
	struct evbuffer	*input = bufferevent_get_input(bev);
	size_t		 len = evbuffer_get_length(input);
	char		 *data = fxcalloc(len + 1, sizeof (char));
	char		 *split;

	evbuffer_remove(input, data, len);

	if (debug)
		fprintf(stderr, "%s: DATA: <%s>, LEN: <%zu>\n", __func__, data, len);

	/* Remove the newline if there is one. */
	if (data[strlen(data) - 1] == '\n')
		data[strlen(data) - 1] = '\0';

	if (*data == '\0')
		goto out;

	split = strtok(data, "\n");
	while (split != NULL) {
		/* XXX: Process message here. */
		split = strtok(NULL, "\n");
	}
out:
	free(data);

	/* XXX: Config option here?
	 *
	 * Disconnect client.
	 */
	TAILQ_REMOVE(&clientq, c, entry);
	bufferevent_free(bev);
}

static void
client_write_cb(struct bufferevent *bev, void *ctx)
{
	struct client	*c = ctx;

	if (debug)
		fprintf(stderr, "Writing... (client %p)...\n", c);
}

static void client_err_cb(struct bufferevent *bev, short events, void *ctx)
{
	struct client	*c = (struct client *)ctx, *clook;

	if (events & (BEV_EVENT_EOF|BEV_EVENT_ERROR)) {
		TAILQ_FOREACH(clook, &clientq, entry) {
			if (c == clook) {
				TAILQ_REMOVE(&clientq, c, entry);
				bufferevent_free(bev);
			}
		}
	}
}

static void
accept_conn_cb(struct evconnlistener *l, evutil_socket_t fd,
		struct sockaddr *add, int socklen, void *ctx)
{
	/* We got a new connection! Set up a bufferevent for it. */
	struct client		*c;
	struct event_base	*base = evconnlistener_get_base(l);

	if (setnonblock(fd) != 0)
		fprintf(stderr, "Couldn't set socket to non-blocking\n");

	c = fxmalloc(sizeof *c);

	c->comms = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

	bufferevent_setcb(c->comms, client_read_cb, client_write_cb,
	    client_err_cb, c);
	bufferevent_enable(c->comms, EV_READ|EV_WRITE);

	TAILQ_INSERT_TAIL(&clientq, c, entry);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));

	event_base_loopexit(base, NULL);
}

void
set_socket_pathname(void)
{
	char		*fpssock_env;
	const char	*unrolled_path;

	fpssock_env = getenv("FVWM_PARSER_SOCKET");
	if (fpssock_env == NULL) {
		xasprintf(&sock_pathname, "%s/%s", getenv("FVWM_USERDIR"),
		    FVWM_PARSER_SOCKET);
		return;
	}

	unrolled_path = expand_path(fpssock_env);
	if (unrolled_path[0] == '/')
		sock_pathname = fxstrdup(unrolled_path);
	else {
		xasprintf(&sock_pathname, "%s/%s", getenv("FVWM_USERDIR"),
			unrolled_path);
	}

	free((void *)unrolled_path);
}

void setup_parser(void)
{
	struct event_base     *base;
	struct evconnlistener *fmd_cfd;
	struct sockaddr_un    sin;

	TAILQ_INIT(&clientq);

	set_socket_pathname();
	unlink(sock_pathname);

	/* Create new event base */
	if ((base = event_base_new()) == NULL) {
		fprintf(stderr, "Couldn't start libevent\n");
		return;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sun_family = AF_LOCAL;
	strcpy(sin.sun_path, sock_pathname);

	/* Create a new listener */
	fmd_cfd = evconnlistener_new_bind(base, accept_conn_cb, NULL,
		  LEV_OPT_CLOSE_ON_FREE, -1,
		  (struct sockaddr *)&sin, sizeof(sin));
	if (fmd_cfd == NULL) {
		perror("Couldn't create listener");
		return;
	}
	evconnlistener_set_error_cb(fmd_cfd, accept_error_cb);

	if (setnonblock(evconnlistener_get_fd(fmd_cfd)) != 0)
		perror("Couldn't set socket to non-blocking");

	chmod(sock_pathname, 0770);

	event_base_dispatch(base);
	unlink(sock_pathname);
}
