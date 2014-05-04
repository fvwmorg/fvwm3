/* -*-c-*- */
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

/* FIXME: The signal handling of this module is largely broken */

#include "config.h"

#include "FvwmConsole.h"
#include "libs/fio.h"

int  s;    /* socket handle */
FILE *sp;
char *name;  /* name of this program at executing time */
char *get_line(void);

/*
 *  close socket and exit
 */
void sclose(int foo)
{
	if (sp != NULL)
	{
		fclose(sp);
		sp = NULL;
	}
	exit(0);

	SIGNAL_RETURN;
}

RETSIGTYPE ReapChildren(int sig)
{
	fvwmReapChildren(sig);
	sclose(sig);

	SIGNAL_RETURN;
}

/*
 * print error message on stderr
 */
void ErrMsg(char *msg)
{
	fprintf(stderr, "%s error in %s: %s\n", name , msg, strerror(errno));
	if (sp != NULL)
	{
		fclose(sp);
		sp = NULL;
	}
	exit(1);
}

/*
 * setup socket.
 * send command to and receive message from the server
 */
int main(int argc, char *argv[])
{
	char *cmd;
	char data[MAX_MESSAGE_SIZE];
	int  len;  /* length of socket address */
	struct sockaddr_un sas;
	int  clen; /* command length */
	int  pid;  /* child process id */
	char *home;
	char *s_name;
	int rc;

	signal(SIGCHLD, ReapChildren);
	signal(SIGPIPE, sclose);
	signal(SIGINT, sclose);
	signal(SIGQUIT, sclose);

	name=strrchr(argv[0], '/');
	if (name != NULL)
	{
		name++;
	}

	/* make a socket */
	home = getenv("FVWM_USERDIR");
	s_name = xmalloc(strlen(home) + sizeof(S_NAME) + 1);
	strcpy(s_name, home);
	strcat(s_name, S_NAME);
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0)
	{
		ErrMsg ("socket");
	}
	/* name the socket and obtain the size of it*/
	sas.sun_family = AF_UNIX;
	strcpy(sas.sun_path, s_name);
	len = sizeof(sas) - sizeof(sas.sun_path) + strlen(sas.sun_path);
	rc = connect(s, (struct sockaddr *)&sas, len);
	if (rc < 0)
	{
		ErrMsg("connect");
	}
	sp = fdopen(s, "r");
	if (sp == NULL)
	{
		ErrMsg("fdopen");
	}
	pid = fork();
	if (pid == -1)
	{
		ErrMsg("fork");
	}
	if (pid == 0)
	{
		/* loop of get user's command and send it to server */
		while (1)
		{
			cmd = get_line();
			if (cmd == NULL)
			{
				break;
			}
			clen = strlen(cmd);
			if (clen == 1)
			{
				/* empty line */
				continue;
			}
			/* send the command including null to the server */
			usleep(1);
			fvwm_send(s, cmd, strlen(cmd) + 1, 0);
		}
		kill(getppid(), SIGKILL);
		sclose(0);
	}
	while (fgets(data, MAX_MESSAGE_SIZE, sp))
	{
		/* get the response */
		/* ignore config lines */
		if (!strcmp(data, C_BEG))
		{
			while (fgets(data, MAX_MESSAGE_SIZE, sp))
			{
				if (*data == '\0' || !strcmp(data,C_END))
				{
					break;
				}
			}
			if (*data != '\0')
			{
				continue;
			}
		}
		if (*data == '\0')
		{
			break;
		}
		printf("%s",data);
	}

	return 0;
}
