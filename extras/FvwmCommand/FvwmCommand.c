/* $Id$
 * $Source$
 *
 * Fvwm command input interface.
 *
 * Copyright 1997, Toshi Isogai.
 * Use this program at your own risk. 
 * Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. 
 *
 */

#include "FvwmCommand.h"

#define MYNAME "FvwmCommand"

int  Fdr, Fdw;  /* file discriptor for fifo */
int  Pfd;
char *getline();
fd_set fdset;

struct timeval Tv;
int  Opt_reply; /* wait for replay */
int  Opt_monitor;     
int  Opt_info;
int  Opt_Serv;
FILE *Fp;
int  Rc;  /* return code */
int  Bg;  /* FvwmCommand in background */

void err_msg( char *msg ); 
void clean_up( void );
void sendit( char *cmd );
void sig_ttin ( int );
void sig_pipe ( int );
void sig_quit ( int );
void usage(void);

void process_message( void ) ;
void list( unsigned long *body, char *) ;
void list_configure(unsigned long *body);
void list_focus_change(unsigned long *body) ;
void list_header(unsigned long *body, char *) ;
void list_icon_loc(unsigned long *body) ;
void list_iconify(unsigned long *body);
void list_mini_icon(unsigned long *body) ;
void list_new_desk(unsigned long *body) ;
void list_new_page(unsigned long *body) ;
void spawn_child( void );


/*******************************************************
 *
 * send command to and receive message from the server 
 *
 *******************************************************/
int main ( int argc, char *argv[]) {
	char cmd[MAX_COMMAND_SIZE];
	char *home;
	char *f_stem, *fc_name, *fm_name;
	int  i;
	int  opt;
	int  ncnt;
	extern char *optarg;
	extern int  optind, opterr, optopt;

	signal (SIGINT, sig_quit);  
	signal (SIGQUIT, sig_quit);  
	signal (SIGTTIN, sig_ttin);

	Opt_reply = 0;
	Opt_info = 1;
	f_stem = NULL;
	Opt_monitor = 0;
	Opt_Serv = 0;
	Tv.tv_sec = 0;
	Tv.tv_usec = 500000;
	Rc = 0;
	Bg = 0;

	while( (opt = getopt( argc, argv, "S:hvf:w:i:rm" )) != EOF ) {
		switch(opt) {
		case 'h':
			usage();
			exit(0);
			break;
		case 'f': 
			f_stem = optarg;
			break;
		case 'S':
			f_stem = optarg;
			Opt_Serv = 1;
			break;
		case 'i':
			Opt_info = atoi( optarg );
			break;
		case 'v':
			printf("%s %s\n", MYNAME, MYVERSION );
			exit(0);
		case 'w':
			Tv.tv_usec = atoi( optarg ) % 1000000;
			Tv.tv_sec = atoi( optarg ) / 1000000;
			break;
		case 'm':
			Opt_monitor = 1;
			break;
		case 'r':
			Opt_reply = 1;
			break;
		case '?':
			exit(3);
		}
	}


	if( f_stem == NULL ) {
		home = getenv("HOME");
		f_stem = safemalloc( strlen(home)+ strlen(F_NAME) + 1);
		strcpy(f_stem,home);
		strcat(f_stem,F_NAME);
	}

	/* create 2 fifos */
	fm_name = safemalloc( strlen(f_stem) + 2 );
	fc_name = safemalloc( strlen(f_stem) + 2 );
	strcpy(fm_name,f_stem);
	strcpy(fc_name,f_stem);
	strcat(fm_name, "M");
	strcat(fc_name, "C");


	if( Opt_Serv ) {
		strcpy( cmd, "FvwmCommand 'FvwmCommandS " );
		strcat( cmd, f_stem );
		strcat( cmd, "'" );
		system( cmd );
	}

	if( ( Fdr= open( fm_name, O_RDONLY  ) ) < 0 ) {
		err_msg( "opening message" );
	}
	if( ( Fdw= open( fc_name, O_WRONLY ) ) < 0 ) {
		err_msg( "opening command" );
	}

	clean_up();

	i = optind;
	if( Opt_monitor ) {

		/* line buffer stdout for pipe */
		setvbuf( stdout, NULL, _IOLBF, 0); 

		/* send arguments first */
		for( ;i < argc; i++ ) {
			strncpy( cmd, argv[i], MAX_COMMAND_SIZE-2 );
			sendit( cmd );
		}
		
		while(1) {
			FD_ZERO(&fdset);
			FD_SET(Fdr, &fdset);
			if( Bg == 0 ) {
				FD_SET(STDIN_FILENO, &fdset); 
			}
			ncnt = _select(FD_SETSIZE,&fdset, 0, 0, NULL);
			
			/* message from fvwm */
			if (FD_ISSET(Fdr, &fdset)){
				process_message();
			}
			
			/* command input */
			if( Bg == 0 ) {
				if( FD_ISSET(STDIN_FILENO, &fdset) ) {
					if( fgets( cmd, MAX_COMMAND_SIZE-2, stdin ) == 0 ) {
						if( Bg == 0 ) {
							/* other than SIGTTIN */
							break;
						}
					}
					sendit( cmd );
				}
			}
		}
	}else {
		if( argc <= i ) {
			/* from stdin */
			while( fgets( cmd, MAX_COMMAND_SIZE-2, stdin ) ) {
				clean_up();
				sendit( cmd );
			}
		} else {	
			/* with arguments */
			for( ;i < argc; i++ ) {
				clean_up();
				strncpy( cmd, argv[i], MAX_COMMAND_SIZE-2 );
				sendit( cmd );
			}
		}
	}
	close(Fdr);
	close(Fdw);
	exit( Rc );
}

/******************************************
 *  signal handlers
 ******************************************/
void sig_quit (int dummy) {
	close(Fdr);
	close(Fdw);
	exit(1);
}
void sig_ttin( int  dummy ) {
	Bg = 1;
	signal( SIGTTIN, SIG_IGN ); 
}

/************************************/
/* print error message on stderr */
/************************************/
void err_msg( char *msg ) {
	fprintf( stderr, "%s error in %s\n", MYNAME , msg );
	fprintf( stderr, "--%s--\n",strerror(errno) );
	close(Fdr);
	close(Fdw);
	exit(1);
}

/*************************************/
/* add cr to the command and send it */
/*************************************/
void sendit( char *cmd ) {
	struct timeval tv;
	int clen;
	int  ncnt;
	
	if( cmd[0] != '\0'  ) {
		clen = strlen(cmd);
		
		/* add cr */
		if( cmd[clen-1] != '\n' ) {
			strcat(cmd, "\n");
			clen++;
		}
		if( clen != 1 ) {
			/* send the command to the server if not empty */
			/* send null too */
			write( Fdw, cmd, clen+1 );
		}

		if( Opt_reply ) {
			/* wait indefinitely */
			FD_ZERO(&fdset);
			FD_SET( Fdr, &fdset);
			ncnt = _select(FD_SETSIZE,&fdset, 0, 0, NULL);
		}
		
		while (1){
			tv.tv_sec = Tv.tv_sec;
			tv.tv_usec = Tv.tv_usec;
			FD_ZERO(&fdset);
			FD_SET(Fdr, &fdset);
			
			ncnt = _select(FD_SETSIZE,&fdset, 0, 0, &tv);

			if( ncnt < 0 ) {
				err_msg( "" );
				break;
			}
			if( ncnt == 0 ) {
				/* timeout */
				break;
			}
			if (FD_ISSET(Fdr, &fdset)) {
				process_message();
			}
		}
	}
}


/******************************************************
 * clean up fifo
 ******************************************************/
void clean_up( void ) {
	int  ncnt;
	char data[80];
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	while (1){
		FD_ZERO(&fdset);
		FD_SET(Fdr, &fdset);
		ncnt = _select(FD_SETSIZE,&fdset, 0, 0, &tv);

		if( ncnt < 0 ) {
			err_msg( "" );
			break;
		}
		if( ncnt == 0 ) {
			break;
		}
		if (FD_ISSET(Fdr, &fdset)){
			ncnt = read( Fdr, data, 80 );
			if( ncnt <= 0 ) {
				break;
			}
		}
	}
}

/*******************************
 * print usage
 *******************************/
void usage(void) {
	printf("Usage: %s [OPTION] [COMMAND]...\n", MYNAME);
	printf("Send commands to fvwm via %sS\n\n", MYNAME);
	printf("  -f <file name>      use fifo <file name> to connect to %sS\n", MYNAME);
	printf("  -i <info level>     0 - error only\n" );
	printf("                      1 - above and config info (default)\n" );
	printf("                      2 - above and static info\n" );
	printf("                      3 - above and dynamic info\n" );
	printf("  -m                  monitor fvwm message transaction\n");
	printf("  -r                  wait for a reply (overrides waiting time)\n");
	printf("  -v                  print version number\n");
	printf("  -w <micro sec>      waiting time for the reponse from fvwm\n");
	printf("\nDefault socket name is ~/.%sSocket\n", MYNAME);
	printf("Default waiting time is 500,000 us\n");
}
 
/*********************************
 *
 * process message
 *
 *********************************/
void process_message( void ) {
	unsigned long type, length, body[24*SOL];
	
 if( (read( Fdr, &type, SOL ) <= 0 ) 
    || (read( Fdr, &length, SOL ) <= 0 )
    || (read( Fdr, body, length ) < (int)length ) ) {
			close(Fdw);
			exit(1);
 }

	if( type==M_ERROR ) {
		fprintf( stderr,"%s", (char *)&body[3] );
		Rc = 1;
	}else if( Opt_info >= 1 ) {

		switch( type ) {

		case M_WINDOW_NAME:
			list( body, "window");
			break;
		case M_ICON_NAME:
			list(body, "icon");
			break;
		case M_RES_CLASS:
			list(body, "class" );
			break;
		case M_RES_NAME:
			list(body, "resource");
			break;
					
		case M_END_WINDOWLIST:
			list_header(body, "end windowlist");
			break;
					
		case M_ICON_FILE:
			list(body, "icon file");
			break;
		case M_ICON_LOCATION:
			list_icon_loc(body);
			break;
					
		case M_END_CONFIG_INFO:
			list_header(body, "end configinfo");
			break;
			
		case M_DEFAULTICON:
			list(body, "default icon");
			break;
			
		case M_MINI_ICON:
			list_mini_icon( body );
			break;
			
		case M_CONFIG_INFO:
			printf( "%s", (char *)&body[3] );
			break;
				
		default:
			if( Opt_info >=2 ) {
				
				switch(type) {
			
				case M_CONFIGURE_WINDOW:
					list_configure( body);
					break;
				case M_STRING:
					list(body, "string");
					break;
			
				default:
					if( Opt_info >= 3 ) {
						switch( type ) {
						case M_NEW_PAGE:
							list_new_page(body);
							break;
						case M_NEW_DESK:
							list_new_desk(body);
							break;
						case M_ADD_WINDOW:
							list_header(body, "add");
							list_configure( body);
							break;
						case M_RAISE_WINDOW:
							list_header(body, "raise");
							break;
						case M_LOWER_WINDOW:
							list_header(body, "lower");
							break;
						case M_FOCUS_CHANGE:
							list_focus_change( body );
							break;
						case M_DESTROY_WINDOW:
							list_header(body, "destroy");
							break;
						case M_ICONIFY:
							list_iconify( body );
							break;
						case M_DEICONIFY:
							list_header(body, "deiconify");
							break;
						case M_MAP:
							list_header(body, "map");
							break;
						case M_WINDOWSHADE:
							list_header(body, "windowshade");
							break;
						case M_DEWINDOWSHADE:
							list_header(body, "dewindowshade");
							break;
						default:
							printf("0x%lx type 0x%lx\n", body[0], type );
						}
					}
				}
			}
		}
	}
}

        			
/**********************************
 *
 *  print configuration info
 *
 **********************************/

void list_configure(unsigned long *body) {
	unsigned long i;
	char *flag;
	char *grav;
	char *flagstr[32] = {
		"StartIconic",
		"OnTop",
		"Sticky",
		"WindowListSkip",
		"SuppressIcon",
		"NoiconTitle",
		"Lenience",
		"StickyIcon",
		"CirculateSkipIcon",
		"CirculateSkip",
		"ClickToFocus",
		"SloppyFocus",
		"SkipMapping",
		"Handles",
		"Title",
		"Mapped",
		"Iconified",
		"Transient",
		"Raised", /* ??? */
		"Visible",
		"IconOurs",
		"PixmapOurs",
		"ShapedIcon",
		"Maximized",
		"WmTakeFocus",
		"WmDeleteWindow",
		"IconMoved",
		"IconUnmapped",
		"MapPending",
		"HintOverride",
		"MWMButtons",
		"MWMBorders"
	 };


	printf( "0x%08lx %-20s x %ld, y %ld, width %ld, height %ld\n",
			body[0], "frame", body[3], body[4], body[5], body[6] );
	printf( "0x%08lx %-20s %ld\n" ,body[0], "desktop", body[7]);

	for( i=0; i<=31; i++ ) {
		if( body[8] & (1<<i) ) {
			flag = "yes";
		}else{
			flag = "no";
		}
		printf( "0x%08lx %-20s %s\n", body[0], flagstr[i], flag );
	}

	printf( "0x%08lx %-20s %ld\n",
			body[0], "title height", body[9]);
	printf( "0x%08lx %-20s %ld\n",
			body[0], "border width", body[10]);
	printf( "0x%08lx %-20s width %ld, height %ld\n",
			body[0], "base size", body[11], body[12]);
	printf( "0x%08lx %-20s width %ld, height %ld\n",
			body[0], "size increment", body[13], body[14]);
	printf( "0x%08lx %-20s width %ld, height %ld\n",
			body[0], "min size", body[15], body[16]);
	printf( "0x%08lx %-20s width %ld, height %ld\n",
			body[0], "max size", body[17], body[18]);
	

	switch(body[21]) {
	case ForgetGravity:
		grav = "Forget";
		break;
    case NorthWestGravity:
		grav = "NorthWest";
		break;
    case NorthGravity:
		grav = "North";
		break;
    case NorthEastGravity:
		grav = "NorthEast";
		break;
    case WestGravity:
		grav = "West";
		break;
    case CenterGravity:
		grav = "Center";
		break;
    case EastGravity:
		grav = "East";
		break;
    case SouthWestGravity:
		grav = "SouthWest";
		break;
    case SouthGravity:
		grav = "South";
		break;
    case SouthEastGravity:
		grav = "SouthEast";
		break;
    case StaticGravity:
		grav = "Static";
		break;
    default:
		grav = "Unknown";
		break;
    }
	printf( "0x%08lx %-20s %s\n", body[0], "gravity", grav);

	printf( "0x%08lx %-20s text 0x%lx, back 0x%lx\n",
			body[0], "pixel", body[22], body[23]);
}

/*************************************************************************
 *
 * Capture  icon location
 *
 ************************************************************************/
void list_icon_loc(unsigned long *body) {

	printf( "0x%08lx %-20s x %ld, y %ld, width %ld, height%ld\n",
			body[0], "icon location", body[3], body[4], body[5], body[6] );
}

/*************************************************************************
 *
 * Capture mini icon
 *
 ************************************************************************/
void list_mini_icon(unsigned long *body) {

	printf( "0x%08lx %-20s width %ld, height %ld, depth %ld\n",
			body[0], "mini icon",body[5], body[6], body[7] );
}

/*************************************************************************
 *
 * Capture  message body[3] 
 *
 ************************************************************************/
void list( unsigned long *body, char *text ) {

	printf( "0x%08lx %-20s %s\n", body[0], text, (char *)&body[3] );
}


/*************************************************************************
 *
 * Capture  new page
 *
 ************************************************************************/
void list_new_page(unsigned long *body) {

	printf( "           %-20s x %ld, y %ld, desk %ld, max x %ld, max y %ld\n",
			"new page", 
			body[0], body[0], body[2], body[3], body[4]);
}

/*************************************************************************
 *
 * Capture  new desk
 *
 ************************************************************************/
void list_new_desk(unsigned long *body) {

	printf( "           %-20s %ld\n",  "new desk", body[0] );
}

/*************************************************************************
 *
 * list header
 *
 ************************************************************************/
void list_header(unsigned long *body, char *text) {

	printf("0x%08lx %s\n", body[0], text); 

}

/*************************************************************************
 *
 * Capture focus change
 *
 ************************************************************************/
void list_focus_change(unsigned long *body) {

	printf( "0x%08lx %-20s highlight 0x%lx, foreground 0x%lx, background 0x%lx\n",
			body[0], "focus change", body[3], body[4], body[5] );
}

/*************************************************************************
 *
 * list iconify
 *
 ************************************************************************/
void list_iconify(unsigned long *body) {

	printf( "0x%08lx %-20s x %ld, y %ld, width %ld, hight %ld\n",
			body[0], "iconify", body[3], body[4], body[5], body[6] );
}

