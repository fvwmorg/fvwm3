/*
** Module.c: code for modules to communicate with fvwm
*/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <errno.h>

#include "fvwmlib.h"
#include "../fvwm/module.h"


/************************************************************************
 *
 * Reads a single packet of info from fvwm. Prototype is:
 * unsigned long header[HEADER_SIZE];
 * unsigned long *body;
 * int fd[2];
 * void DeadPipe(int nonsense);  * Called if the pipe is no longer open  *
 *
 * ReadFvwmPacket(fd[1],header, &body);
 *
 * Returns:
 *   > 0 everything is OK.
 *   = 0 invalid packet.
 *   < 0 pipe is dead. (Should never occur)
 *   body is a malloc'ed space which needs to be freed
 *
 **************************************************************************/
int ReadFvwmPacket(int fd, unsigned long *header, unsigned long **body)
{
  int count,total,count2,body_length;
  char *cbody;
  extern RETSIGTYPE DeadPipe(int);

  errno = 0;
  if((count = read(fd,header,HEADER_SIZE*sizeof(unsigned long))) >0)
  {
    if(header[0] == START_FLAG)
    {
      body_length = header[2]-HEADER_SIZE;
      *body = (unsigned long *)
        safemalloc(body_length * sizeof(unsigned long));
      cbody = (char *)(*body);
      total = 0;
      while(total < body_length*sizeof(unsigned long))
      {
        errno = 0;
        if((count2=
            read(fd,&cbody[total],
                 body_length*sizeof(unsigned long)-total)) >0)
        {
          total += count2;
        }
        else if(count2 < 0)
        {
          DeadPipe(errno);
        }
      }
    }
    else
      count = 0;
  }
  if(count <= 0)
    DeadPipe(errno);
  return count;
}

/************************************************************************
 *
 * SendText - Sends arbitrary text/command back to fvwm
 *
 ***********************************************************************/
void SendText(int *fd,char *message,unsigned long window)
{
  int w;

  if(message != NULL)
  {
    write(fd[0],&window, sizeof(unsigned long));

    w=strlen(message);
    write(fd[0],&w,sizeof(int));
    if (w)
      write(fd[0],message,w);

    /* keep going */
    w = 1;
    write(fd[0],&w,sizeof(int));
  }
}

/***************************************************************************
 *
 * Sets the which-message-types-do-I-want mask for modules
 *
 **************************************************************************/
void SetMessageMask(int *fd, unsigned long mask)
{
  char set_mask_mesg[50];

  sprintf(set_mask_mesg,"SET_MASK %lu\n",mask);
  SendText(fd,set_mask_mesg,0);
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
  static int first_pass = 1;
  int count,done = 0;
  int body_size;
  static char *line = NULL;
  unsigned long header[HEADER_SIZE];

  if(line != NULL)
    free(line);

  if(first_pass)
  {
    SendInfo(fd,"Send_ConfigInfo",0);
    first_pass = 0;
  }

  while(!done)
  {
    count = ReadFvwmPacket(fd[1],header,(unsigned long **)&line);
    /* DB(("Packet count is %d", count)); */
    if (count <= 0)
      *tline = NULL;
    else {
      *tline = &line[3*sizeof(long)];
      body_size = header[2]-HEADER_SIZE;
      /* DB(("Config line (%d): `%s'", body_size, body_size ? *tline : "")); */
      while((body_size > 0)
            && isspace(**tline)) {
        (*tline)++;
        --body_size;
      }
    }

/*   fprintf(stderr,"%x %x\n",header[1],M_END_CONFIG_INFO);*/
    if(header[1] == M_CONFIG_INFO)
      done = 1;
    else if(header[1] == M_END_CONFIG_INFO)
    {
      done = 1;
      if(line != NULL)
        free(line);
      line = NULL;
      *tline = NULL;
    }
  }
}
