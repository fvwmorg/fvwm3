/* FvwmMFL -- Fvwm3 Module Front Loader
 *
 * This program accepts listeners over a UDS for the purposes of receiving
 * information from FVWM3.
 *
 * Released under the same license as FVWM3 itself.
 */

#include "config.h"

#include "fvwm/fvwm.h"

#include "libs/Module.h"
#include "libs/safemalloc.h"
#include "libs/queue.h"
#include "libs/fvwmsignal.h"
#include "libs/vpacket.h"
#include "libs/getpwuid.h"
#include "libs/envvar.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__sun__)
#include <libbson-1.0/bson.h>
#else
#include <bson/bson.h>
#endif

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

/* This is also configurable via getenv() - see FvwmMFL(1) */
#define MFL_SOCKET_DEFAULT "fvwm_mfl.sock"
#define MYNAME "FvwmMFL"

static int debug;
struct fvwm_msg;
static char *sock_pathname;
static char *pid_file;

struct client {
	struct bufferevent	*comms;
	unsigned long		 flags_m;
	unsigned long		 flags_mx;
	struct fvwm_msg		*fm;

	TAILQ_ENTRY(client)	 entry;
};

TAILQ_HEAD(clients, client);
struct clients  clientq;

struct fvwm_comms {
	int		 fd[2];
	struct event	*read_ev;
	ModuleArgs	*m;
};
struct fvwm_comms	 fc;

struct fvwm_msg {
	bson_t	*msg;
	int	 fw;
};

struct event_flag {
	const char	*event;
	unsigned long	 flag;
} etf[] = {
	{"new_window", M_ADD_WINDOW},
	{"configure_window", M_CONFIGURE_WINDOW},
	{"new_page", M_NEW_PAGE},
	{"new_desk", M_NEW_DESK},
	{"raise_window", M_RAISE_WINDOW},
	{"lower_window", M_LOWER_WINDOW},
	{"focus_change", M_FOCUS_CHANGE},
	{"destroy_window", M_DESTROY_WINDOW},
	{"iconify", M_ICONIFY},
	{"deiconify", M_DEICONIFY},
	{"window_name", M_WINDOW_NAME},
	{"visible_name", M_VISIBLE_NAME},
	{"icon_name", M_ICON_NAME},
	{"res_class", M_RES_CLASS},
	{"res_name", M_RES_NAME},
	{"icon_location", M_ICON_LOCATION},
	{"map", M_MAP},
	{"icon_file", M_ICON_FILE},
	{"window_shade", M_WINDOWSHADE},
	{"dewindow_shade", M_DEWINDOWSHADE},
	{"restack", M_RESTACK},
	{"leave_window", MX_LEAVE_WINDOW|M_EXTENDED_MSG},
	{"enter_window", MX_ENTER_WINDOW|M_EXTENDED_MSG},
	{"visible_icon_name", MX_VISIBLE_ICON_NAME|M_EXTENDED_MSG},
	{"echo", MX_ECHO|M_EXTENDED_MSG},
};


static void fvwm_read(int, short, void *);
static void broadcast_to_client(FvwmPacket *);
static void setup_signal_handlers(struct event_base *);
static inline const char *flag_to_event(unsigned long);
static void HandleTerminate(int, short, void *);
static int client_set_interest(struct client *, const char *);
static struct fvwm_msg *handle_packet(unsigned long, unsigned long *, unsigned long);
static struct fvwm_msg *fvwm_msg_new(void);
static void fvwm_msg_free(struct fvwm_msg *);
static void register_interest(void);
static void send_version_info(struct client *);
static void set_socket_pathname(void);
static int check_pid(void);
static void set_pid_file(void);
static void create_pid_file(void);
static void delete_pid_file(void);

static void
delete_pid_file(void)
{
	if (pid_file == NULL)
		return;

	unlink(pid_file);
}

static void
set_pid_file(void)
{
	char	*fud = getenv("FVWM_USERDIR");
	char	*dsp = getenv("DISPLAY");

	if (fud == NULL || dsp == NULL) {
		fprintf(stderr,
		    "FVWM_USERDIR or DISPLAY is not set in the environment\n");
		exit(1);
	}

	free(pid_file);
	xasprintf(&pid_file, "%s/fvwm_mfl_%s.pid", fud, dsp);
}

static void
create_pid_file(void)
{
	FILE	*pf;

	if ((pf = fopen(pid_file, "w")) == NULL) {
		fprintf(stderr, "Couldn't open %s because: %s\n", pid_file,
			strerror(errno));
		exit(1);
	}
	fprintf(pf, "%d", getpid());

	if (fclose(pf) != 0) {
		fprintf(stderr, "Couldn't close %s because: %s\n", pid_file,
			strerror(errno));
		exit(1);
	}

	/* Alongside sigTerm, ensure we remove the pid when exiting as well. */
	atexit(delete_pid_file);
}

static int
check_pid(void)
{
	FILE	*pf;
	int	 pid;

	if (pid_file == NULL)
		set_pid_file();

	if (access(pid_file, F_OK) != 0) {
		create_pid_file();
		return (1);
	}

	if ((pf = fopen(pid_file, "r")) == NULL) {
		fprintf(stderr, "Couldn't open %s for reading: %s\n", pid_file,
			strerror(errno));
		exit(1);
	}
	fscanf(pf, "%d", &pid);

	/* Non-fatal if we can't close this file handle from reading. */
	(void)fclose(pf);

	if ((kill(pid, 0) == 0)) {
		fprintf(stderr, "FvwmMFL is already running\n");
		return (0);
	}
	return (1);
}


static struct fvwm_msg *
fvwm_msg_new(void)
{
	struct fvwm_msg		*fm;

	fm = fxcalloc(1, sizeof *fm);

	return (fm);
}

static void
fvwm_msg_free(struct fvwm_msg *fm)
{
	bson_destroy(fm->msg);
	free(fm);
}

static void
HandleTerminate(int fd, short what, void *arg)
{
	fprintf(stderr, "%s: dying...\n", __func__);
	delete_pid_file();
	unlink(sock_pathname);
	fvwmSetTerminate(fd);
}

static void
setup_signal_handlers(struct event_base *base)
{
	struct event	*hup, *term, *intrp, *quit, *pipe, *child;


	hup   =  evsignal_new(base, SIGHUP,  HandleTerminate, NULL);
	term  =  evsignal_new(base, SIGTERM, HandleTerminate, NULL);
	quit  =  evsignal_new(base, SIGQUIT, HandleTerminate, NULL);
	pipe  =  evsignal_new(base, SIGPIPE, HandleTerminate, NULL);
	child =  evsignal_new(base, SIGCHLD, HandleTerminate, NULL);
	intrp =  evsignal_new(base, SIGINT,  HandleTerminate, NULL);

	evsignal_add(hup, NULL);
	evsignal_add(term, NULL);
	evsignal_add(quit, NULL);
	evsignal_add(pipe, NULL);
	evsignal_add(child, NULL);
	evsignal_add(intrp, NULL);
}

static void
send_version_info(struct client *c)
{
	struct fvwm_msg		*fm;
	size_t			 json_len;
	char			*as_json, *to_client;

	fm = fvwm_msg_new();
	fm->msg = BCON_NEW("connection_profile",
		"{",
		    "version", BCON_UTF8(VERSION),
		    "version_info", BCON_UTF8(VERSIONINFO),
		"}"
	);
	as_json = bson_as_relaxed_extended_json(fm->msg, &json_len);
	if (as_json == NULL) {
		free(fm);
		return;
	}

	/* Ensure there's a newline at the end of the string, so that
	 * buffered output can be sent.
	 */
	asprintf(&to_client, "%s\n", as_json);

	bufferevent_write(c->comms, to_client, strlen(to_client));
	bufferevent_flush(c->comms, EV_WRITE, BEV_NORMAL);
	free(fm);
	free(to_client);
}

static struct fvwm_msg *
handle_packet(unsigned long type, unsigned long *body, unsigned long len)
{
	struct fvwm_msg		*fm = NULL;
	const char		*type_name = flag_to_event(type);
	char			 xwid[20];

	if (type_name == NULL) {
		fprintf(stderr, "Couldn't find type_name\n");
		goto out;
	}

	fm = fvwm_msg_new();

	sprintf(xwid, "0x%lx", body[0]);

	if (debug)
		fprintf(stderr, "%s: matched: <<%s>>\n", __func__, type_name);

	switch(type) {
	case MX_ENTER_WINDOW:
	case MX_LEAVE_WINDOW: {
		fm->msg = BCON_NEW(type_name,
		  "{",
		      "window", BCON_UTF8(xwid),
		  "}"
		);
		return (fm);
	}
	case MX_ECHO: {
		fm->msg = BCON_NEW(type_name,
		  "{",
		    "message", BCON_UTF8((char *)&body[3]),
		  "}"
		);
		return (fm);
	}
	case M_ADD_WINDOW:
	case M_CONFIGURE_WINDOW: {
		struct ConfigWinPacket *cwp = (void *)body;
		fm->msg = BCON_NEW(type_name,
		  "{",
		    "window", BCON_UTF8(xwid),
		    "title_height", BCON_INT64(cwp->title_height),
		    "border_width", BCON_INT64(cwp->border_width),
		    "frame", "{",
		      "window", BCON_INT64(cwp->frame),
		      "x", BCON_INT64(cwp->frame_x),
		      "y", BCON_INT64(cwp->frame_y),
		      "width", BCON_INT64(cwp->frame_width),
		      "height", BCON_INT64(cwp->frame_height),
		    "}",
		    "hints",
		    "{",
			"base_width", BCON_INT64(cwp->hints_base_width),
			"base_height", BCON_INT64(cwp->hints_base_height),
			"inc_width", BCON_INT64(cwp->hints_width_inc),
			"inc_height", BCON_INT64(cwp->hints_height_inc),
			"orig_inc_width", BCON_INT64(cwp->orig_hints_width_inc),
			"orig_inc_height", BCON_INT64(cwp->orig_hints_height_inc),
			"min_width", BCON_INT64(cwp->hints_min_width),
			"min_height", BCON_INT64(cwp->hints_min_height),
			"max_width", BCON_INT64(cwp->hints_max_width),
			"max_height", BCON_INT64(cwp->hints_max_height),
		     "}",
		     "ewmh",
		      "{",
		        "layer", BCON_INT64(cwp->ewmh_hint_layer),
			"desktop", BCON_INT64(cwp->ewmh_hint_desktop),
			"window_type", BCON_INT64(cwp->ewmh_window_type),
		      "}",
		  "}"
		);
		return (fm);
	}
	case M_MAP:
	case M_LOWER_WINDOW:
	case M_RAISE_WINDOW:
	case M_DESTROY_WINDOW: {
		fm->msg = BCON_NEW(type_name,
		  "{",
		      "window", BCON_UTF8(xwid),
		  "}"
		);
		return (fm);
	}
	case M_FOCUS_CHANGE: {
		fm->msg = BCON_NEW(type_name,
		  "{",
		      "window", BCON_UTF8(xwid),
		      "type", BCON_INT64(body[2]),
		      "hilight",
		      "{",
		          "text_colour", BCON_INT64(body[3]),
			  "bg_colour", BCON_INT64(body[4]),
		      "}",
		  "}"
		);
		return (fm);
	}
	case M_WINDOW_NAME:
	case M_VISIBLE_NAME:
	case MX_VISIBLE_ICON_NAME:
	case M_ICON_NAME:
	case M_RES_CLASS:
	case M_RES_NAME: {
		fm->msg = BCON_NEW(type_name,
		  "{",
		      "window", BCON_UTF8(xwid),
		      "name", BCON_UTF8((char *)&body[3]),
		  "}"
		);

		return (fm);
	}
	case M_NEW_DESK: {
		fm->msg = BCON_NEW(type_name,
		  "{",
		      "desk", BCON_INT64(body[0]),
		      "monitor_id", BCON_INT32(body[1]),
		  "}"
		);
		return (fm);
	}
	case M_NEW_PAGE: {
		fm->msg = BCON_NEW(type_name,
		  "{",
		      "virtual_scr",
		      "{",
		          "vx", BCON_INT64(body[0]),
			  "vy", BCON_INT64(body[1]),
			  "vx_pages", BCON_INT64(body[5]),
			  "vy_pages", BCON_INT64(body[6]),
			  "current_desk", BCON_INT64(body[2]),
		       "}",
		       "display",
		       "{",
		           "width", BCON_INT64(body[3]),
			   "height", BCON_INT64(body[4]),
		        "}",
			"monitor_id", BCON_INT32(body[7]),
		  "}"
		);
		return (fm);
	}
	case M_ICON_LOCATION: {
		fm->msg = BCON_NEW(type_name,
		  "{",
		      "window", BCON_UTF8(xwid),
		      "x", BCON_INT64(body[3]),
		      "y", BCON_INT64(body[4]),
		      "width", BCON_INT64(body[5]),
		      "height", BCON_INT64(body[6]),
		   "}"
		);
		return (fm);
	}
	case M_ICONIFY: {
		fm->msg = BCON_NEW(type_name,
		  "{",
		      "window", BCON_UTF8(xwid),
		      "icon",
		      "{",
		          "x", BCON_INT64(body[3]),
		          "y", BCON_INT64(body[4]),
		          "width", BCON_INT64(body[5]),
		          "height", BCON_INT64(body[6]),
		       "}",
		       "frame",
		       "{",
		           "x", BCON_INT64(body[7]),
			   "y", BCON_INT64(body[8]),
			   "width", BCON_INT64(body[9]),
			   "height", BCON_INT64(body[1]),
		       "}"
		);
		return (fm);
	}
	default:
		goto out;
	}
out:
	fvwm_msg_free(fm);
	return (NULL);
}

static inline const char *
flag_to_event(unsigned long flag)
{
	size_t	 i;
	bool is_extended = (flag & M_EXTENDED_MSG);

	for (i = 0; i < (sizeof(etf) / sizeof(etf[0])); i++) {
		int f = etf[i].flag;
		if (is_extended && (f & M_EXTENDED_MSG) && (f == flag))
			return (etf[i].event);
		if (!is_extended && (f & flag))
			return (etf[i].event);
	}

	return (NULL);
}

static inline bool
strsw(const char *pre, const char *str)
{
	return (strncmp(pre, str, strlen(pre)) == 0);
}

#define EFLAGSET	0x1
#define EFLAGUNSET	0x2

static int
client_set_interest(struct client *c, const char *event)
{
	size_t		 i;
	int		 flag_type = 0;
	bool		 changed = false;
#define PRESET "set"
#define PREUNSET "unset"

	if (strsw(PRESET, event)) {
		event += strlen(PRESET) + 1;
		flag_type = EFLAGSET;
	} else if (strsw(PREUNSET, event)) {
		event += strlen(PREUNSET) + 1;
		flag_type = EFLAGUNSET;
	}

	if (strcmp(event, "debug") == 0) {
		debug = (flag_type == EFLAGSET) ? 1 : 0;

		/* Never send to FVWM3. */
		return (true);
	}

	for (i = 0; i < (sizeof(etf) / sizeof(etf[0])); i++) {
		if (strcmp(etf[i].event, event) == 0) {
			unsigned long	 f = etf[i].flag;

			changed = true;
			if (flag_type == EFLAGSET) {
				if (f & M_EXTENDED_MSG) {
					if (debug)
						fprintf(stderr,
						    "Setting %s\n", event);
					c->flags_mx |= f;
				} else {
					if (debug)
						fprintf(stderr,
						    "Setting %s\n", event);
					c->flags_m |= f;
				}
			} else {
				if (f & M_EXTENDED_MSG) {
					if (debug)
						fprintf(stderr,
						    "Unsetting %s\n", event);
					c->flags_mx &= ~f;
				} else {
					if (debug)
						fprintf(stderr,
						   "Unsetting %s\n", event);
					c->flags_m &= ~f;
				}
			}
		}
	}
	return (changed);
}

static void
register_interest(void)
{
	size_t		i;
	unsigned long	f_m = 0, f_mx = 0;

	for (i = 0; i < (sizeof(etf) / sizeof(etf[0])); i++) {
		unsigned long f = etf[i].flag;
		if (f & M_EXTENDED_MSG)
			f_mx |= f;
		else
			f_m |=f;
	}

	if (debug) {
		fprintf(stderr, "Sending: flags_m:  %lu\n", f_m);
		fprintf(stderr, "Sending: flags_mx: %lu\n", f_mx);
	}
	SetMessageMask(fc.fd, f_m);
	SetMessageMask(fc.fd, f_mx);
}

static void
broadcast_to_client(FvwmPacket *packet)
{
	struct client		*c;
	struct fvwm_msg		*fm;
	char			*as_json, *to_client;
	size_t			 json_len;
	unsigned long		*body = packet->body;
	unsigned long		 type =	packet->type;
	unsigned long		 length = packet->size;
	bool			 is_extended = (type & M_EXTENDED_MSG);
	bool			 enact_change = false;

	TAILQ_FOREACH(c, &clientq, entry) {
		if (is_extended) {
			if (c->flags_mx == type)
				enact_change = true;
			else
				enact_change = false;
		}
		if (!is_extended || !enact_change) {
			if (c->flags_m & type)
				enact_change = true;
			else
				enact_change = false;
		}
		/* This packet really isn't for this client; move on to the
		 * next client.
		 */
		if (!enact_change)
			continue;

		if ((fm = handle_packet(type, body, length)) == NULL) {
			fprintf(stderr, "Packet was NULL...\n");
			continue;
		}

		as_json = bson_as_relaxed_extended_json(fm->msg, &json_len);
		if (as_json == NULL) {
			free(fm);
			continue;
		}
		c->fm = fm;

		/* Ensure there's a newline at the end of the string, so that
		 * buffered output can be sent.
		 */
		asprintf(&to_client, "%s\n", as_json);

		bufferevent_write(c->comms, to_client, strlen(to_client));
		bufferevent_flush(c->comms, EV_WRITE, BEV_NORMAL);
		free(fm);
		free(to_client);
	}
}

static void
client_read_cb(struct bufferevent *bev, void *ctx)
{
	struct evbuffer	*input = bufferevent_get_input(bev);
	struct client	*c =	 ctx;
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
		if (client_set_interest(c, split)) {
			split = strtok(NULL, "\n");
			continue;
		}
		SendText(fc.fd, split, c->fm ? c->fm->fw : 0);
		split = strtok(NULL, "\n");
	}
out:
	free(data);
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

	c = fxmalloc(sizeof *c);

	c->comms = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	c->flags_m = 0;
	c->flags_mx = 0;
	c->fm = NULL;

	bufferevent_setcb(c->comms, client_read_cb, client_write_cb,
	    client_err_cb, c);
	bufferevent_enable(c->comms, EV_READ|EV_WRITE|EV_PERSIST);

	send_version_info(c);

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

static void
fvwm_read(int efd, short ev, void *data)
{
	FvwmPacket	*packet;

	if ((packet = ReadFvwmPacket(efd)) == NULL) {
		if (debug)
			fprintf(stderr, "Couldn't read from FVWM - exiting.\n");
		unlink(sock_pathname);
		exit(0);
	}

	broadcast_to_client(packet);
}

void
set_socket_pathname(void)
{
	char		*mflsock_env, *tmpdir;
	const char	*unrolled_path;

	/* Figure out if we are using default MFL socket path or we should
	 * respect environment variable FVWMMFL_SOCKET for FvwmMFL socket path
	 */

	mflsock_env = getenv("FVWMMFL_SOCKET");
	if (mflsock_env == NULL) {
		/* Check if TMPDIR is defined.  If so, use that, otherwise
		 * default to /tmp for the directory name.
		 */
		if ((tmpdir = getenv("TMPDIR")) == NULL)
			tmpdir = "/tmp";
		xasprintf(&sock_pathname, "%s/%s", tmpdir, MFL_SOCKET_DEFAULT);
		return;
	}

	unrolled_path = expand_path(mflsock_env);
	if (unrolled_path[0] == '/')
		sock_pathname = fxstrdup(unrolled_path);
	else {
		xasprintf(&sock_pathname, "%s/%s", getenv("FVWM_USERDIR"),
			unrolled_path);
	}

	free((void *)unrolled_path);
}

int main(int argc, char **argv)
{
	struct event_base     *base;
	struct evconnlistener *fmd_cfd;
	struct sockaddr_un    sin;

	TAILQ_INIT(&clientq);

	if ((fc.m = ParseModuleArgs(argc, argv, 1)) == NULL) {
		fprintf(stderr, "%s must be started by FVWM3\n", MYNAME);
		return (1);
	}

	/* If we're already running... */
	if (check_pid() == 0)
		return (1);

	set_socket_pathname();
	unlink(sock_pathname);

	/* Create new event base */
	if ((base = event_base_new()) == NULL) {
		fprintf(stderr, "Couldn't start libevent\n");
		return (1);
	}
	setup_signal_handlers(base);

	memset(&sin, 0, sizeof(sin));
	sin.sun_family = AF_LOCAL;
	strcpy(sin.sun_path, sock_pathname);

	/* Create a new listener */
	fmd_cfd = evconnlistener_new_bind(base, accept_conn_cb, NULL,
		  LEV_OPT_CLOSE_ON_FREE, -1,
		  (struct sockaddr *)&sin, sizeof(sin));
	if (fmd_cfd == NULL) {
		perror("Couldn't create listener");
		return 1;
	}
	evconnlistener_set_error_cb(fmd_cfd, accept_error_cb);

	chmod(sock_pathname, 0770);

	/* Setup comms to fvwm3. */
	fc.fd[0] = fc.m->to_fvwm;
	fc.fd[1] = fc.m->from_fvwm;

	/* If the user hasn't set FVWMMFL_SOCKET in the environment, then tell
	 * Fvwm3 to do this.  Note that we have to get Fvwm3 to do this so
	 * that all other windows can inherit this environment variable.  We
	 * cannot use flib_putenv() here since this never reaches Fvwm3's
	 * environment.
	 */
	if (getenv("FVWMMFL_SOCKET") == NULL) {
		char	*pe;

		xasprintf(&pe, "SetEnv FVWMMFL_SOCKET %s", sock_pathname);
		SendText(fc.fd, pe, 0);
		free(pe);
	}

	if (evutil_make_socket_nonblocking(fc.fd[0]) < 0)
		fprintf(stderr, "fvwm to_fvwm socket non-blocking failed");
	if (evutil_make_socket_nonblocking(fc.fd[1]) < 0)
		fprintf(stderr, "fvwm to_fvwm socket non-blocking failed");

	fc.read_ev = event_new(base, fc.fd[1], EV_READ|EV_PERSIST, fvwm_read, NULL);
	event_add(fc.read_ev, NULL);

	register_interest();

	SendFinishedStartupNotification(fc.fd);

	event_base_dispatch(base);
	unlink(sock_pathname);

	return (0);
}
