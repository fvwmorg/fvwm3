/* $Id$
 * $Source$
 *
 * Fvwm command input interface.
 *
 * Copyright 1997, Toshi Isogai. No guarantees or warantees or anything
 * are provided. Use this program at your own risk. Permission to use 
 * this program for any purpose is given,
 * as long as the copyright is kept intact. 
 *
 */

#include "FvwmCommand.h"

#define MYNAME    "FvwmCommandS"

int  Fd[2];  /* pipe to fvwm */
int  Ffdr, Ffdw; /* fifo file discriptors */
char *Fc_name, *Fm_name;  /* fifo names */
int  Nounlink;   /* don't delete fifo when true */

void close_pipes();
void DeadPipe( int );
void err_msg( char *msg );
void process_message(unsigned long type,unsigned long *body);
void relay_packet( unsigned long, unsigned long, unsigned long *);
void server( char * );
void sig_handler( int );

void main(int argc, char *argv[]){
	char *fifoname;

	if(argc < FARGS)    {
		fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MYNAME,
				MYVERSION);
		exit(1);
	}

	if( argc == FARGS+1 ) {
		fifoname = argv[FARGS];
	}else{
		fifoname = NULL;
	}


/*	signal (SIGPIPE, DeadPipe);   */
	signal (SIGINT, sig_handler);  
	signal (SIGQUIT, sig_handler);  
	signal (SIGHUP, sig_handler);  
	signal (SIGTERM, sig_handler);  

	Fd[0] = atoi(argv[1]);
	Fd[1] = atoi(argv[2]);

	Nounlink = 0;

	server( fifoname );
}

/***********************************************************************
 *	signal handler
 ***********************************************************************/
void DeadPipe( int dummy ) {
	/* how do I know if fvwm's pipe or Command's pipe */
	fprintf(stderr,"%s: dead pipe\n", MYNAME);
	close_pipes();
	exit(0);

}

void sig_handler(int dummy) {
	close_pipes();
	exit(1);
}

/*********************************************************
 * close  pipes
 *********************************************************/
void close_pipes() {

	close(Fd[0]);
	close(Fd[1]);
	close(Ffdr);
	close(Ffdw);

	if( !Nounlink ) {
	  unlink(Fc_name); 
	  unlink(Fm_name); 
	}
}

/*********************************************************/
/* setup server and communicate with fvwm and the client */
/*********************************************************/
void server ( char *name ) {
	char buf[MAX_COMMAND_SIZE];      /*  command line buffer */
	char *home;
	char *f_stem;
	int  len;
	fd_set fdset;
	unsigned long *body;
	unsigned long header[HEADER_SIZE];

	if( name == NULL ) {
		/* default name */
		home = getenv("HOME");
		f_stem = safemalloc( strlen(home)+ strlen(F_NAME) + 1);
		strcpy(f_stem,home);
		strcat(f_stem,F_NAME);

	}else{
		f_stem = name;
	}


	/* create 2 fifos */
	Fc_name = safemalloc( strlen(f_stem) + 2 );
	Fm_name = safemalloc( strlen(f_stem) + 2 );
	strcpy(Fc_name,f_stem);
	strcpy(Fm_name,f_stem);
	strcat(Fc_name, "C");
	strcat(Fm_name, "M");

	if( (Ffdw = open(Fc_name, O_RDWR | O_NONBLOCK ) ) > 0) {
		write( Ffdw, "killme #nounlink\n", 18 );  
		close( Ffdw );
	}

	unlink( Fm_name ); 
	unlink( Fc_name ); 

	if( mkfifo( Fm_name, S_IRUSR | S_IWUSR ) < 0 ) {
		err_msg( "creating message fifo" );
	}
	if( mkfifo( Fc_name, S_IRUSR | S_IWUSR ) < 0 ) {
		err_msg( "creating command fifo" );
	}

	SendText(Fd,"",0); /* tell fvwm that we are here */

	while(1) {
	if( (Ffdr = open(Fc_name, O_RDWR | O_NONBLOCK | O_TRUNC) ) < 0 ) {
		err_msg( "opening command" );
	}
	if( (Ffdw = open(Fm_name, O_RDWR | O_NONBLOCK | O_TRUNC) ) < 0 ) {
		err_msg( "opening message" );
	}

		while (1){

			FD_ZERO(&fdset);

			FD_SET(Ffdr, &fdset);
			FD_SET(Fd[1], &fdset);

			_select(FD_SETSIZE,&fdset, 0, 0, NULL);

			if (FD_ISSET(Fd[1], &fdset)){
				if( ReadFvwmPacket(Fd[1],header,&body) > 0)	 {
					process_message(header[1], body);
					free(body);
				}
			}
			if (FD_ISSET(Ffdr, &fdset)){
				if( (len = read( Ffdr, buf, MAX_COMMAND_SIZE )) <= 0 ) {
					break;
				}
				if( !strcmp( buf, "killme #nounlink\n" ) ) {
				  Nounlink = 1;
				  strcpy( buf, "killme\n" );
				}

				SendText(Fd,buf,0); /* send command */
			}
		}
		close( Ffdr );
		close( Ffdw );
	}
}


/**************************************************************************
 *
 * Process window list messages 
 *
 *************************************************************************/
void process_message(unsigned long type,unsigned long *body){
	int  msglen;

	switch(type) {

    case M_CONFIGURE_WINDOW:
		relay_packet( type, 24*SOL, body );
		break;

    case M_WINDOW_NAME:
    case M_ICON_NAME:
    case M_RES_CLASS:
    case M_RES_NAME:
    case M_ICON_FILE:
	case M_DEFAULTICON:
		msglen = strlen( (char *)&body[3] );
		relay_packet( type, msglen+1+3*SOL, body );
		break;

    case M_END_WINDOWLIST:
	case M_END_CONFIG_INFO:
		relay_packet( type, 0*SOL, body );
		break;

    case M_ICON_LOCATION:
		relay_packet( type, 7*SOL, body );
		break;

    case M_ERROR:
	case M_STRING:
	case M_CONFIG_INFO:
		msglen = strlen( (char *)&body[3] );
		relay_packet( type, msglen+1+3*SOL, body );
		break;


	case M_MINI_ICON:
		relay_packet( type, 6*SOL, body );
		break;
		

	case M_NEW_PAGE:
		relay_packet( type, 5*SOL, body );
		break;
		
	case M_NEW_DESK:
		relay_packet( type, 1*SOL, body );
		break;

	case M_ADD_WINDOW:
		relay_packet( type, 24*SOL, body );
				break;
				
	case M_RAISE_WINDOW:
	case M_LOWER_WINDOW:
	case M_FOCUS_CHANGE:
	case M_DESTROY_WINDOW:
	case M_DEICONIFY:
	case M_MAP:
	case M_WINDOWSHADE:
	case M_DEWINDOWSHADE:
		relay_packet( type, 4*SOL, body );
		break;
		
	case M_ICONIFY:
		relay_packet( type, 7*SOL, body );

		break;
		
	default:
		relay_packet( type, 4*SOL, body );
    }
}



/******************************************/
/* print error message on stderr and exit */
/******************************************/
void err_msg( char *msg ) {
	fprintf( stderr, "%s server error in %s, errno %d\n", MYNAME, msg, errno );
	close_pipes();
	exit(1);
}


/*
 * relay packet to front-end
 */
void relay_packet( unsigned long type, unsigned long length, unsigned long *body) {

	write( Ffdw, &type, SOL );
	write( Ffdw, &length, SOL );
	write( Ffdw, body, length );
}

