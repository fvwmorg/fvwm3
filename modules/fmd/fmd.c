/* fmd - fvwm module daemon
 *
 * This program accepts listeners over a UDS for the purposes of receiving
 * information from FVWM3.
 *
 * Released under the same license as FVWM3 itself.
 */

#include "config.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/queue.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <signal.h>

#include <bson.h>
#include <event.h>

#include "libs/Module.h"
#include "libs/safemalloc.h"
#include "libs/queue.h"
#include "libs/fvwmsignal.h"

#define SOCKET_PATH "/tmp/fvwm_fmd.sock"
#define BUFLEN 1024

const char *myname = "fmd";
struct event fvwm_read, fvwm_write;
static ModuleArgs *module;
static int fvwm_fd[2];

static int setnonblock(int);

static void on_client_read(int, short, void *);
static void on_client_write(int, short, void *);
static void on_client_accept(int, short, void *);

static void on_fvwm_read(int, short, void *);
static void on_fvwm_write(int, short, void *);

static void broadcast_to_client(unsigned long);

static void setup_signal_handlers(void);
static RETSIGTYPE HandleTerminate(int sig);

static RETSIGTYPE
HandleTerminate(int sig)
{
	fvwmSetTerminate(sig);
	exit(0);
	SIGNAL_RETURN;
}

static void
setup_signal_handlers(void)
{
    struct sigaction  sigact;

    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGTERM);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGPIPE);
#ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT; /* to interrupt ReadFvwmPacket() */
#else
    sigact.sa_flags = 0;
#endif
    sigact.sa_handler = HandleTerminate;

    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGINT,  &sigact, NULL);
    sigaction(SIGPIPE, &sigact, NULL);
    signal(SIGTERM, HandleTerminate);
    signal(SIGINT,  HandleTerminate);
    signal(SIGPIPE, HandleTerminate);     /* Dead pipe == fvwm died */
#ifdef HAVE_SIGINTERRUPT
    siginterrupt(SIGTERM, True);
    siginterrupt(SIGINT,  True);
    siginterrupt(SIGPIPE, True);
#endif
}

struct client {
	struct event	read, write;
	u_int		flags;

	TAILQ_ENTRY(client)  entry;
	TAILQ_HEAD(, buffer) writeq;
};
TAILQ_HEAD(clients, client);
struct clients	clientq;

struct buffer {
	int 	 offset, len;
	char 	*buf;

	TAILQ_ENTRY(buffer) entries;
};

static int
setnonblock(int fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL)) < 0)
		return (flags);

	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0)
		return (-1);

	return (0);
}

static void
broadcast_to_client(unsigned long type)
{
	struct client	*c;
	struct buffer 	*bufferq;
	char 		*buf;

	TAILQ_FOREACH(c, &clientq, entry) {
		if (!(c->flags & type))
			continue;
		buf = fxstrdup("Got you a window");

		bufferq = fxcalloc(1, sizeof(*bufferq));
		bufferq->len = strlen(buf);
		bufferq->buf = fxstrdup(buf);
		bufferq->offset = 0;
		TAILQ_INSERT_TAIL(&c->writeq, bufferq, entries);

		free(buf);

		event_add(&c->write, NULL);
	}
}

static void
on_fvwm_read(int fd, short ev, void *arg)
{
	FvwmPacket	*packet;

	if ((packet = ReadFvwmPacket(fd)) == NULL) {
		fprintf(stderr, "Couldn't read from FVWM - exiting.\n");
		close(fd);
		event_del(&fvwm_read);

		exit(1);
	}

	switch(packet->type) {
	case M_ADD_WINDOW:
		broadcast_to_client(packet->type);
		break;
	default:
		break;
	}
	SendUnlockNotification(fvwm_fd);
}

static void
on_fvwm_write(int fd, short ev, void *arg)
{
	return;
}

static void
on_client_read(int fd, short ev, void *arg)
{
	struct client 	*client = (struct client *)arg;
	char		*buf;
	int		 len;

	buf = fxmalloc(BUFLEN);

	if ((len = read(fd, buf, BUFLEN)) <= 0) {
		free(buf);
		fprintf(stderr, "client disconnected.\n");
		close(fd);
		event_del(&client->read);
		TAILQ_REMOVE(&clientq, client, entry);
		free(client);
		return;
	}

	/* Remove the newline if there is one. */
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';

	if (*buf == '\0') {
		free(buf);
		return;
	}

	if (strcmp(buf, "set new_window") == 0)
		client->flags |= M_ADD_WINDOW;

	free(buf);
}

static void
on_client_write(int fd, short ev, void *arg)
{
	struct client	*client = (struct client *)arg;
	struct buffer	*bufferq;
	int		 len;

	if ((bufferq = TAILQ_FIRST(&client->writeq)) == NULL)
		return;

	/* Write the buffer.  A portion of the buffer may have been
	 * written in a previous write, so only write the remaining
	 * bytes.
	 */

	len = bufferq->len - bufferq->offset;
	len = write(fd, bufferq->buf + bufferq->offset,
			bufferq->len - bufferq->offset);
	if (len == -1) {
		if (errno == EINTR || errno == EAGAIN) {
			/* Write failed. */
			event_add(&client->write, NULL);
			return;
		} else
			err(1, "write failed");
	} else if ((bufferq->offset + len) < bufferq->len) {
		/* Incomplete write - reschedule. */
		bufferq->offset += len;
		event_add(&client->write, NULL);
		return;
	}

	/* Write complete.  Remove from the buffer. */
	TAILQ_REMOVE(&client->writeq, bufferq, entries);
	free(bufferq->buf);
	free(bufferq);
}

static void
on_client_accept(int fd, short ev, void *arg)
{
	struct sockaddr_in 	 client_addr;
	struct client 		*client;
	socklen_t 		 client_len = sizeof(client_addr);
	int 			 client_fd;

	/* Accept the new connection. */
	client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd == -1) {
		fprintf(stderr, "accept failed: %s", strerror(errno));
		return;
	}

	if (setnonblock(client_fd) < 0)
		fprintf(stderr, "couldn't make socket non-blocking\n");

	client = fxcalloc(1, sizeof(*client));

	event_set(&client->read, client_fd, EV_READ|EV_PERSIST, on_client_read,
	    client);

	/* Make the event active, by adding it. */
	event_add(&client->read, NULL);

	/* Setting the event here makes libevent aware of it, but we don't
	 * want to make it active yet via event_add() until we're ready.
	 */
	event_set(&client->write, client_fd, EV_WRITE, on_client_write, client);

	TAILQ_INIT(&client->writeq);
	TAILQ_INSERT_TAIL(&clientq, client, entry);

	fprintf(stderr, "Accepted connection...\n");
}

int
main(int argc, char **argv)
{
	struct sockaddr_un addr;
	struct event ev_accept;
	int fd;

	if ((module = ParseModuleArgs(argc, argv, 1)) == NULL) {
		fprintf(stderr, "%s must be started by FVWM3\n", myname);
		exit(1);
	}

	setup_signal_handlers();

	fvwm_fd[0] = module->to_fvwm;
	fvwm_fd[1] = module->from_fvwm;

	event_init();

	TAILQ_INIT(&clientq);

	/* Set up the read/write listeners as early as possible, as there's
	 * every chance FVWM will send us information as soon as we start up.
	 */
	event_set(&fvwm_read, fvwm_fd[1], EV_READ|EV_PERSIST, on_fvwm_read, NULL);
	event_add(&fvwm_read, NULL);

	event_set(&fvwm_write, fvwm_fd[0], EV_WRITE, on_fvwm_write, NULL);

	SendFinishedStartupNotification(fvwm_fd);
	SetSyncMask(fvwm_fd, M_ADD_WINDOW);

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		err(1, "listen failed");

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	if (unlink(addr.sun_path) != 0 && errno != ENOENT)
		err(1, "unlink failed");

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		err(1, "bind failed");
	if (listen(fd, 1024) < 0)
		err(1, "listen failed");
	if (setnonblock(fd) < 0)
		err(1, "server socket non-blocking failed");

	event_set(&ev_accept, fd, EV_READ|EV_PERSIST, on_client_accept, NULL);
	event_add(&ev_accept, NULL);

	fprintf(stderr, "Started.  Waiting for connections...\n");

	event_dispatch();

	return (0);
}
