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

/*
** Module.c: code for modules to communicate with fvwm
*/
#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include "Module.h"
#include "safemalloc.h"


/*
 * Loop until count bytes are read, unless an error or end-of-file
 * condition occurs.
 */
inline
static int positive_read(int fd, char* buf, int count )
{
    while ( count > 0 ) {
	int n_read = read( fd, buf, count );
	if ( n_read <= 0 )
	    return -1;
	buf += n_read;
	count -= n_read;
    }
    return 0;
}


/**
 * Reads a single packet of info from FVWM.
 * The packet is stored in static memory that is reused during
 * the next call.
 **/

FvwmPacket* ReadFvwmPacket( int fd )
{
    static unsigned long buffer[FvwmPacketMaxSize];
    FvwmPacket* packet = (FvwmPacket*)buffer;

    /* The `start flag' value supposedly exists to synchronize the
       FVWM -> module communication.  However, the communication goes
       through a pipe.  I don't see how any data could ever get lost, so
       how would FVWM & the module become unsynchronized?
    */
    do {
	if ( positive_read( fd, (char *)buffer, sizeof(unsigned long) ) < 0 )
	    return NULL;
    } while (packet->start_pattern != START_FLAG);

    /* Now read the rest of the header */
    if ( positive_read( fd, (char *)(&buffer[1]),
			3 * sizeof(unsigned long) ) < 0 )
	return NULL;

    /* Finally, read the body, and we're done */
    if ( positive_read( fd, (char *)(&buffer[4]),
			FvwmPacketBodySize(*packet)*sizeof(unsigned long)) < 0)
	return NULL;

    return packet;
}


/************************************************************************
 *
 * SendFinishedStartupNotification - informs fvwm that the module has
 * finished its startup procedures and is fully operational now.
 *
 ***********************************************************************/
void SendFinishedStartupNotification(int *fd)
{
  SendText(fd, "FINISHED_STARTUP", 0);
}

/************************************************************************
 *
 * SendText - Sends arbitrary text/command back to fvwm
 *
 ***********************************************************************/
void SendText(int *fd, char *message, unsigned long window)
{
  char *p, *buf;
  unsigned int len;

  if (!message)
    return;

  /* Get enough memory to store the entire message.
   */
  len = strlen(message);
  p = buf = alloca(sizeof(long) * (3 + 1 + (len / sizeof(long))));

  /* Put the message in the buffer, and... */
  *((unsigned long *)p) = window;
  p += sizeof(unsigned long);

  *((unsigned long *)p) = len;
  p += sizeof(unsigned long);

  strcpy(p, message);
  p += len;

  len = 1;
  memcpy(p, &len, sizeof(unsigned long));
  p += sizeof(unsigned long);

  /* Send it!  */
  write(fd[0], buf, p - buf);
}


void SetMessageMask(int *fd, unsigned long mask)
{
  char set_mask_mesg[50];

  sprintf(set_mask_mesg,"SET_MASK %lu\n",mask);
  SendText(fd,set_mask_mesg,0);
}


/*
 * Optional routine that sets the matching criteria for config lines
 * that should be sent to a module by way of the GetConfigLine function.
 *
 * If this routine is not called, all module config lines are sent.
 */
static int first_pass = 1;

void InitGetConfigLine(int *fd,char *match)
{
  char *buffer = (char *)alloca(strlen(match) + 32);
  first_pass = 0;                       /* make sure get wont do this */
  sprintf(buffer,"Send_ConfigInfo %s",match);
  SendText(fd,buffer,0);
}


/***************************************************************************
 * Gets a module configuration line from fvwm. Returns NULL if there are
 * no more lines to be had. "line" is a pointer to a char *.
 *
 * Changed 10/19/98 by Dan Espen:
 *
 * - The "isspace"  call was referring to  memory  beyond the end of  the
 * input area.  This could have led to the creation of a core file. Added
 * "body_size" to keep it in bounds.
 **************************************************************************/
void GetConfigLine(int *fd, char **tline)
{
    FvwmPacket* packet;
    int body_count;

    if (first_pass) {
	SendText(fd,"Send_ConfigInfo",0);
	first_pass = 0;
    }

    do {
	packet = ReadFvwmPacket( fd[1] );
	if ( packet == NULL || packet->type == M_END_CONFIG_INFO ) {
	    *tline = NULL;
	    return;
	}
    } while ( packet->type != M_CONFIG_INFO );

    /* For whatever reason CONFIG_INFO packets start with three
       (unsigned long) zeros.  Skip the zeros and any whitespace that
       follows */
    *tline = (char*)&(packet->body[3]);
    body_count = FvwmPacketBodySize(*packet) * sizeof(unsigned long);

    while ( body_count > 0 && isspace((unsigned char)**tline) ) {
        (*tline)++;
        --body_count;
    }
}


ModuleArgs* ParseModuleArgs( int argc, char* argv[], int use_arg6_as_alias )
{
    static ModuleArgs ma;

    /* Need at least six arguments:
       [0] name of executable
       [1] file descriptor of module->FVWM pipe (write end)
       [2] file descriptor of FVWM->module pipe (read end)
       [3] pathname of last config file read (ignored -- use Send_ConfigInfo)
       [4] application window context
       [5] window decoration context

       Optionally (left column used if use_arg6_as_alias is true):
       [6] alias       or  user argument 0
       [7] user arg 0  or  user arg 1
       ...
    */
    if ( argc < 6 ) return NULL;

    /* Module name is (last component of) argv[0] or possibly an alias
       passed on the command line. */
    if ( use_arg6_as_alias && argc >= 7 ) {
	ma.name = argv[6];
	ma.user_argc = argc - 7;
	ma.user_argv = &(argv[7]);
    } else {
	char* p = strrchr( argv[0], '/' );
	if ( p == NULL )
	    ma.name = argv[0];
	else
	    ma.name = ++p;
	ma.user_argc = argc - 6;
	ma.user_argv = &(argv[6]);
    }

    if ( ma.user_argc == 0 )
	ma.user_argv = NULL;

    /* File descriptors for the pipes */
    ma.to_fvwm = atoi( argv[1] );
    ma.from_fvwm = atoi( argv[2] );

    /* Ignore argv[3] */

    /* These two are generated as long hex strings */
    ma.window = strtoul( argv[4], NULL, 16 );
    ma.decoration = strtoul( argv[5], NULL, 16 );

    return &ma;
}
