/*
  Fvwm command input interface.

  Copyright 1996, Toshi Isogai. No guarantees or warantees or anything
  are provided. Use this program at your own risk. Permission to use
  this program for any purpose is given,
  as long as the copyright is kept intact.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "FvwmConsole.h"

char *MyName;

int Fd[2];  /* pipe to fvwm */
int  Ns;             /* socket handles */
char Name[80]; /* name of this program in executable format */
char *S_name;  /* socket name */

void server( void );
void DeadPipe( int );
void CloseSocket();
void ErrMsg( char *msg );
void SigHandler( int );

int main(int argc, char *argv[])
{
  char *tmp, *s;
  char client[120];
  char **eargv;
  int i,j,k;
  char *xterm_pre[] = { "-title", Name, "-name", Name, NULL };
  char *xterm_post[] = { "-e", NULL, NULL };
  int  clpid;

  /* Why is this not just put in the initializer of xterm_a?
     Apparently, it is a non-standard extension to use a non-constant address
     * (of client) in an initializer (of xterm_a). */
  xterm_post[1] = client;

  /* Save the program name - its used for error messages and option parsing */
  tmp = argv[0];

  s=strrchr(argv[0], '/');
  if (s != NULL)
    tmp = s + 1;

  strcpy( Name, tmp );

  MyName = safemalloc(strlen(tmp)+2);
  strcpy(MyName,"*");
  strcat(MyName, tmp);

  /* construct client's name */
  strcpy( client, argv[0] );
  strcat( client, "C" );

  if(argc < FARGS)    {
	fprintf(stderr,"%s version %s should only be executed by fvwm!\n",
		MyName, VERSION);
	exit(1);
  }

  if( ( eargv =(char **)safemalloc((argc+12)*sizeof(char *)) ) == NULL ) {
	ErrMsg( "allocation" );
  }

  /* copy arguments */
  eargv[0] = XTERM;
  j = 1;
  for ( k=0 ; xterm_pre[k] != NULL ; j++, k++ ) {
	eargv[j] = xterm_pre[k];
  }

  for ( i=FARGS ; i<argc; i++ ) {
	if( !strcmp ( argv[i], "-e" ) ) {
	  i++;
	  break;
	} else if( !strcmp ( argv[i], "-terminal" ) ) {
	  i++;
	  if (i < argc)
	    /* use alternative terminal emulator */
	    eargv[0] = argv[i];
	} else {
	  eargv[j++] = argv[i];
	}
  }

  for ( k=0 ; xterm_post[k] != NULL ; j++, k++ ) {
	eargv[j] = xterm_post[k];
  }

  /* copy rest of -e args */
  for(  ; i<argc; i++, j++ ) {
	  eargv[j-1] = argv[i];
  }

  eargv[j] = NULL;

  /* Dead pipes mean fvwm died */
  signal (SIGPIPE, DeadPipe);
  signal (SIGINT, SigHandler);
  signal (SIGQUIT, SigHandler);

  Fd[0] = atoi(argv[1]);
  Fd[1] = atoi(argv[2]);

  /* launch xterm with client */
  clpid = fork();
  if( clpid < 0) {
	ErrMsg("client forking");
  }else if(clpid  == 0 ) {
	execvp( *eargv, eargv );
	ErrMsg("exec");
  }

  /* tell fvwm we're running */
  SendFinishedStartupNotification(Fd);

  server();
  return (0);
}

/***********************************************************************
 *	signal handler
 ***********************************************************************/
void DeadPipe( int dummy )
{
  CloseSocket();
  exit(0);
}

void SigHandler(int dummy)
{
  CloseSocket();
  exit(1);
}

/*********************************************************/
/* close sockets and spawned process                     */
/*********************************************************/
void CloseSocket()
{
  send(Ns, C_CLOSE, strlen(C_CLOSE), 0);
  close(Ns);     /* remove the socket */
  unlink( S_name );

}

/*********************************************************/
/* setup server and communicate with fvwm and the client */
/*********************************************************/
void server ( void )
{
  struct sockaddr_un sas, csas;
  int  len;
  size_t clen;     /* length of sockaddr */
  char buf[MAX_COMMAND_SIZE];      /*  command line buffer */
  char *tline;
  char ver[40];
  fd_set fdset;
  char *home;
  int s;
  int msglen;

  /* make a socket  */
  if( (s = socket(AF_UNIX, SOCK_STREAM, 0 )) < 0  ) {
	ErrMsg( "socket");
	exit(1);
  }

  /* name the socket */
  home = getenv("FVWM_USERDIR");
  S_name = safemalloc(strlen(home) + sizeof(S_NAME) + 1);
  strcpy(S_name, home);
  strcat(S_name, S_NAME);

  sas.sun_family = AF_UNIX;
  strcpy( sas.sun_path, S_name );

  /* bind the above name to the socket */
  /* first, erase the old socket */
  unlink( S_name );
  len = sizeof(sas) - sizeof( sas.sun_path) + strlen( sas.sun_path );

  if( bind(s, (struct sockaddr *)&sas,len) < 0 ) {
	ErrMsg( "bind" );
	exit(1);
  }

  /* listen to the socket */
  /* set backlog to 5 */
  if ( listen(s,5) < 0 ) {
    ErrMsg( "listen" );
	exit(1);
  }

  /* accept connections */
  clen = sizeof(csas);
  if(( Ns = accept(s, (struct sockaddr *)&csas, &clen)) < 0 ) {
	ErrMsg( "accept");
	exit(1);
  }

  /* send config lines to Client */
  tline = NULL;
  send(Ns, C_BEG, strlen(C_BEG), 0);
  GetConfigLine(Fd,&tline);
  while(tline != NULL) {
	if(strlen(tline)>1) {
	  send(Ns, tline, strlen(tline),0);
	  send(Ns, "\n", 1, 0);
	}
	GetConfigLine(Fd,&tline);
  }
  send(Ns, C_END, strlen(C_END), 0);
  strcpy( ver, MyName);
  strcat( ver, " version " );
  strcat( ver, VERSION);
  strcat( ver, "\n" );
  send(Ns, ver, strlen(ver), 0 );

  while (1){
      FD_ZERO(&fdset);
      FD_SET(Ns, &fdset);
      FD_SET(Fd[1], &fdset);

      select(FD_SETSIZE, SELECT_FD_SET_CAST &fdset, 0, 0, NULL);

      if (FD_ISSET(Fd[1], &fdset)){
	  FvwmPacket* packet = ReadFvwmPacket(Fd[1]);
	  if ( packet == NULL ) {
	      CloseSocket();
	      exit(0);
	  } else {
	      if (packet->type == M_PASS) {
		  msglen = strlen((char *)&(packet->body[3]));
		  if ( msglen > MAX_MESSAGE_SIZE-2 ) {
		      msglen = MAX_MESSAGE_SIZE-2;
		  }
		  send( Ns, (char *)&(packet->body[3]), msglen, 0 );
	      }
	  }
      }

      if (FD_ISSET(Ns, &fdset)){
	  if( recv( Ns, buf, MAX_COMMAND_SIZE,0 ) == 0 ) {
	    /* client is terminated */
	    close(Ns);
	    unlink(S_name);
	    exit(0);
	  }

	  /* process the own unique commands */
	  SendText(Fd,buf,0); /* send command */
      }
  }
}

/******************************************/
/* print error message on stderr and exit */
/******************************************/
void ErrMsg( char *msg )
{
  fprintf( stderr, "%s server error in %s, errno %d\n", Name, msg, errno );
  CloseSocket();
  exit(1);
}
