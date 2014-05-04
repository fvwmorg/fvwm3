/* -*-c-*- */
/*
 * Fvwm command input interface.
 *
 * Copyright 1996, Toshi Isogai. No guarantees or warantees or anything
 * are provided. Use this program at your own risk. Permission to use
 * this program for any purpose is given,
 * as long as the copyright is kept intact.
 */

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
#include "libs/fvwmsignal.h"

/* using ParseModuleArgs now */
static ModuleArgs* module;

int Fd[2];  /* pipe to fvwm */
int  Ns = -1; /* socket handles */
char Name[80]; /* name of this program in executable format */
char *S_name = NULL;  /* socket name */

void server(void);
RETSIGTYPE DeadPipe(int);
void ErrMsg(char *msg);
void SigHandler(int);

void clean_up(void)
{
	if (Ns != -1)
	{
		close(Ns);
		Ns = -1;
	}
	if (S_name != NULL)
	{
		unlink(S_name);
		S_name = NULL;
	}

	return;
}

/*
 *      signal handler
 */
RETSIGTYPE DeadPipe(int dummy)
{
	clean_up();
	exit(0);

	SIGNAL_RETURN;
}

RETSIGTYPE SigHandler(int dummy)
{
	clean_up();
	exit(0);

	SIGNAL_RETURN;
}

RETSIGTYPE ReapChildren(int sig)
{
	fvwmReapChildren(sig);

	SIGNAL_RETURN;
}

int main(int argc, char *argv[])
{
	char client[120];
	char **eargv;
	int i, j, k;
	char *xterm_pre[] = { "-title", Name, "-name", Name, NULL };
	char *xterm_post[] = { "-e", NULL, NULL };
	int  clpid;

	/* Why is this not just put in the initializer of xterm_a?
	 * Apparently, it is a non-standard extension to use a non-constant
	 * address (of client) in an initializer (of xterm_a).
	 */
	xterm_post[1] = client;

	module = ParseModuleArgs(argc, argv, 0); /* we don't use an alias */
	if (module == NULL)
	{
		fprintf(
			stderr, "FvwmConsole version "VERSION" should only be"
			" executed by fvwm!\n");
		exit(1);
	}
	strcpy(Name, module->name);
	/* construct client's name */
	strcpy(client, argv[0]);
	strcat(client, "C");
	eargv = xmalloc((argc+12)*sizeof(char *));
	/* copy arguments */
	eargv[0] = XTERM;
	j = 1;
	for (k = 0; xterm_pre[k] != NULL; j++, k++)
	{
		eargv[j] = xterm_pre[k];
	}
	for (i = 0; i< module->user_argc; i++)
	{
		if (!strcmp(module->user_argv[i], "-e"))
		{
			i++;
			break;
		}
		else if (!strcmp(module->user_argv[i], "-terminal"))
		{
			i++;
			if (i < module->user_argc)
				/* use alternative terminal emulator */
				eargv[0] = module->user_argv[i];
		}
		else
		{
			eargv[j++] = module->user_argv[i];
		}
	}
	for (k = 0; xterm_post[k] != NULL; j++, k++)
	{
		eargv[j] = xterm_post[k];
	}

	/* copy rest of -e args */
	for (; i<module->user_argc; i++, j++)
	{
		eargv[j-1] = module->user_argv[i];
	}
	eargv[j] = NULL;
	signal(SIGCHLD, ReapChildren);
	/* Dead pipes mean fvwm died */
	signal(SIGPIPE, DeadPipe);
	signal(SIGINT, SigHandler);
	signal(SIGQUIT, SigHandler);
	signal(SIGALRM, SigHandler);

	Fd[0] = module->to_fvwm;
	Fd[1] = module->from_fvwm;

	/* launch xterm with client */
	clpid = fork();
	if (clpid < 0)
	{
		ErrMsg("client forking");
	}
	else if (clpid  == 0)
	{
		execvp(*eargv, eargv);
		ErrMsg("exec");
	}
	SetMessageMask(Fd, M_END_CONFIG_INFO | M_ERROR);
	SetMessageMask(Fd, M_EXTENDED_MSG);
	/* tell fvwm we're running */
	SendFinishedStartupNotification(Fd);

	server();

	return 0;
}

/*
 * setup server and communicate with fvwm and the client
 */
void server(void)
{
	struct sockaddr_un sas, csas;
	int  len;
	socklen_t clen;     /* length of sockaddr */
	char buf[MAX_COMMAND_SIZE];      /*  command line buffer */
	char *tline;
	char ver[40];
	fd_set fdset;
	char *home;
	int s;
	int msglen;
	int rc;

	/* make a socket  */
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0 )
	{
		ErrMsg("socket");
	}

	/* name the socket */
	home = getenv("FVWM_USERDIR");
	S_name = xmalloc(strlen(home) + sizeof(S_NAME) + 1);
	strcpy(S_name, home);
	strcat(S_name, S_NAME);

	sas.sun_family = AF_UNIX;
	strcpy(sas.sun_path, S_name);
	/* bind the above name to the socket: first, erase the old socket */
	unlink(S_name);
	len = sizeof(sas) - sizeof(sas.sun_path) + strlen(sas.sun_path);
	umask(0077);
	rc = bind(s, (struct sockaddr *)&sas, len);
	if (rc < 0)
	{
		ErrMsg("bind");
	}
	else
	{
		/* don't wait forever for connections */
		alarm(FVWMCONSOLE_CONNECTION_TO_SECS);
		/* listen to the socket */
		rc = listen(s, 5);
		alarm(0);
		if (rc < 0)
		{
			ErrMsg("listen");
		}
		/* accept connections */
		clen = sizeof(csas);
		Ns = accept(s, (struct sockaddr *)&csas, &clen);
		if (Ns < 0)
		{
			ErrMsg("accept");
		}
	}
	/* send config lines to Client */
	tline = NULL;
	fvwm_send(Ns, C_BEG, strlen(C_BEG), 0);
	GetConfigLine(Fd, &tline);
	while (tline != NULL)
	{
		if (strlen(tline) > 1)
		{
			fvwm_send(Ns, tline, strlen(tline), 0);
			fvwm_send(Ns, "\n", 1, 0);
		}
		GetConfigLine(Fd, &tline);
	}
	fvwm_send(Ns, C_END, strlen(C_END), 0);
	strcpy(ver, "*");
	strcpy(ver, module->name);
	strcat(ver, " version ");
	strcat(ver, VERSION VERSIONINFO);
	strcat(ver, "\n");
	fvwm_send(Ns, ver, strlen(ver), 0);

	while (1)
	{
		FD_ZERO(&fdset);
		FD_SET(Ns, &fdset);
		FD_SET(Fd[1], &fdset);

		fvwmSelect(FD_SETSIZE, SELECT_FD_SET_CAST &fdset, 0, 0, NULL);
		if (FD_ISSET(Fd[1], &fdset))
		{
			FvwmPacket* packet = ReadFvwmPacket(Fd[1]);

			if (packet == NULL)
			{
				clean_up();
				exit(0);
			}
			else if (packet->type == M_ERROR)
			{
				msglen = strlen((char *)&(packet->body[3]));
				if (msglen > MAX_MESSAGE_SIZE-2)
				{
					msglen = MAX_MESSAGE_SIZE-2;
				}
				fvwm_send(
					Ns, (char *)&(packet->body[3]), msglen,
					0);
			}
		}
		if (FD_ISSET(Ns, &fdset))
		{
			int len;
			int rc;

			rc = fvwm_recv(Ns, buf, MAX_COMMAND_SIZE, 0);
			if (rc == 0)
			{
				/* client is terminated */
				clean_up();
				exit(0);
			}
			/* process the own unique commands */
			len = strlen(buf);
			if (len > 0 && buf[len - 1] == '\n')
			{
				buf[len - 1] = '\0';
			}
			/* send command */
			SendText(Fd, buf, 0);
		}
	}
}

/*
 * print error message on stderr and exit
 */
void ErrMsg(char *msg)
{
	fprintf(
		stderr, "%s server error in %s, errno %d: %s\n", Name, msg,
		errno, strerror(errno));
	clean_up();
	exit(1);
}
