#include "FvwmConsole.h"

int  s;    /* socket handle */
FILE *sp;
char *name;  /* name of this program at executing time */
char *getline();


/******************************************/
/*  close socket and exit */
/******************************************/
void sclose () {
  fclose(sp);
  exit(0);
}

/************************************/
/* print error message on stderr */
/************************************/
void ErrMsg( char *msg ) {
  fprintf( stderr, "%s error in %s\n", name , msg );
  fclose(sp);
  exit(1);
}


/*******************************************************/
/* setup socket.                                       */
/* send command to and receive message from the server */
/*******************************************************/
void main ( int argc, char *argv[]) {
  char *cmd;
  unsigned char data[BUFSIZE];
  int  len;  /* length of socket address */
  struct sockaddr_un sas;
  int  clen; /* command length */
  int  pid;  /* child process id */
  
  signal (SIGINT, sclose);  
  signal (SIGQUIT, sclose);  

  name=strrchr(argv[0], '/');
  if (name != NULL) {
    name++;
  }

  /* make a socket */
  if( (s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ) {
	ErrMsg ("socket");
  }

  /* name the socket and obtain the size of it*/
  sas.sun_family = AF_UNIX;
  strcpy( sas.sun_path, S_NAME );
  len = sizeof(sas.sun_family) + strlen(sas.sun_path);

  if( connect( s, (struct sockaddr *)&sas, len )< 0 ) {
	ErrMsg( "connect" );
  }

  sp = fdopen( s, "r" );

  pid = fork();
  if( pid == -1 ) {
	ErrMsg( "fork");
  }
  if( pid == 0 ) {
	/* loop of get user's command and send it to server */
	while( 1 ) {

	  cmd = getline();
	  if( cmd == NULL  ) {
		break;
	  }
	
	  clen = strlen(cmd);
	  if( clen == 1 ) {
		continue;    /* empty line */
	  }

	  /* send the command to the server */
	  send( s, cmd, strlen(cmd), 0 );

	}
	kill( getppid(), SIGKILL );
	sclose();
  }
  while( fgets( data, BUFSIZE, sp )  ) {
	/* get the response */
	/* ignore config lines */
	if( !strcmp( data, C_BEG ) ) {
	  while( fgets( data, BUFSIZE, sp) ) {
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
}

