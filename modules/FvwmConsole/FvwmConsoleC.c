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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* FIXME: The signal handling of this module is largely broken */

#include "config.h"

#include "FvwmConsole.h"

int  s;    /* socket handle */
FILE *sp;
char *name;  /* name of this program at executing time */
char *getline();

/*
 *  close socket and exit
 */
void sclose (int foo)
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

	exit(0);
	SIGNAL_RETURN;
}

/*
 * print error message on stderr
 */
void ErrMsg( char *msg )
{
  fprintf(stderr, "%s error in %s: %s\n", name , msg, strerror(errno));
  if (sp != NULL) {
	  fclose(sp);
	  sp = NULL;
  }
  exit(1);
}


/*
 * setup socket.
 * send command to and receive message from the server
 */
int main( int argc, char *argv[])
{
  char *cmd;
  char data[MAX_MESSAGE_SIZE];
  int  len;  /* length of socket address */
  struct sockaddr_un sas;
  int  clen; /* command length */
  int  pid;  /* child process id */
  char *home;
  char *s_name;

  signal(SIGCHLD, ReapChildren);
  signal(SIGPIPE, sclose);
  signal(SIGINT, sclose);
  signal(SIGQUIT, sclose);

  name=strrchr(argv[0], '/');
  if (name != NULL) {
    name++;
  }

  /* make a socket */
  home = getenv("FVWM_USERDIR");
  s_name = safemalloc( strlen(home) + sizeof(S_NAME) + 1);
  strcpy(s_name, home);
  strcat(s_name, S_NAME);
  if( (s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ) {
	ErrMsg ("socket");
  }

  /* name the socket and obtain the size of it*/
  sas.sun_family = AF_UNIX;
  strcpy( sas.sun_path, s_name );
  len = sizeof(sas) - sizeof( sas.sun_path) + strlen( sas.sun_path );

  if( connect( s, (struct sockaddr *)&sas, len )< 0 ) {
	ErrMsg( "connect" );
  }

  sp = fdopen( s, "r" );
  if (sp == NULL) {
	ErrMsg( "fdopen");
  }

  pid = fork();
  if( pid == -1 ) {
	ErrMsg( "fork");
  }
  if( pid == 0 ) {
	/* loop of get user's command and send it to server */
	while( 1 ) {

	  cmd = getline();
	  if (cmd == NULL) {
		break;
	  }

	  clen = strlen(cmd);
	  if( clen == 1 ) {
		continue;    /* empty line */
	  }

	  /* send the command including null to the server */
	  usleep(1);
	  send( s, cmd, strlen(cmd)+1, 0 );

	}
	kill( getppid(), SIGKILL );
	sclose(0);
  }
  while( fgets( data, MAX_MESSAGE_SIZE, sp )  ) {
	/* get the response */
	/* ignore config lines */
	if( !strcmp( data, C_BEG ) ) {
	  while( fgets( data, MAX_MESSAGE_SIZE, sp) ) {
		if( *data == '\0' || !strcmp(data,C_END) ) {
		  break;
		}
	  }
	  if( *data != '\0' ) {
		continue;
	  }
	}
	if( *data == '\0' ) {
	  break;
	}
	printf( "%s",data );
  }
  return (0);
}
