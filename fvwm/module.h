#ifndef MODULE_H
#define MODULE_H

struct queue_buff_struct
{
  struct queue_buff_struct *next;
  unsigned long *data;
  int size;
  int done;
};

extern int npipes;
extern int *readPipes;
extern int *writePipes;
extern struct queue_buff_struct **pipeQueue;

#define START_FLAG 0xffffffff

#define M_NEW_PAGE           (1)
#define M_NEW_DESK           (1<<1)
#define M_ADD_WINDOW         (1<<2)
#define M_RAISE_WINDOW       (1<<3)
#define M_LOWER_WINDOW       (1<<4)
#define M_CONFIGURE_WINDOW   (1<<5)
#define M_FOCUS_CHANGE       (1<<6)
#define M_DESTROY_WINDOW     (1<<7)
#define M_ICONIFY            (1<<8)
#define M_DEICONIFY          (1<<9)
#define M_WINDOW_NAME        (1<<10)
#define M_ICON_NAME          (1<<11)
#define M_RES_CLASS          (1<<12)
#define M_RES_NAME           (1<<13)
#define M_END_WINDOWLIST     (1<<14)
#define M_ICON_LOCATION      (1<<15)
#define M_MAP                (1<<16)

/* It turns out this is defined by <sys/stream.h> on Solaris 2.6.
   I suspect that simply redefining this will lead to trouble; 
   at some point, these should probably be renamed (FVWM_MSG_ERROR?). */
#ifdef M_ERROR
#  undef M_ERROR
#endif
#define M_ERROR              (1<<17)

#define M_CONFIG_INFO        (1<<18)
#define M_END_CONFIG_INFO    (1<<19)
#define M_ICON_FILE          (1<<20)
#define M_DEFAULTICON        (1<<21)
#define M_STRING             (1<<22)
#define M_MINI_ICON          (1<<23)
#define M_WINDOWSHADE        (1<<24)
#define M_DEWINDOWSHADE      (1<<25)
#define M_LOCKONSEND         (1<<26)
#define M_SENDCONFIG         (1<<27)
#define MAX_MESSAGES         28

/*
 * MAX_MASK is used to initialize the pipeMask array.  In a few places
 * this is used along with  other module arrays to   see if a pipe  is
 * active.
 *
 * The stuff about not turning on lock on send is from Afterstep.
 *
 * I needed sendconfig off  to  identify open  pipes that want  config
 * info messages while active.
 *
 * There really should be  a   module structure.  Ie.  the   "readPipes",
 * "writePipes", "pipeName", arrays  should be  members  of a  structure.
 * Probably a linklist of structures.  Note that if the OS number of file
 * descriptors   gets really  large,   the  current  architecture  starts
 * creating and looping  over  large arrays.  The  impact seems  to be in
 * module.c, modconf.c and event.c.  dje 10/2/98
 */
#define MAX_MASK             (((1<<MAX_MESSAGES)-1)\
                              &~(M_LOCKONSEND + M_SENDCONFIG))

/*
 * M_LOCKONSEND  when set causes fvwm to  wait for the  module to send an
 * .unlock  message back, needless  to say, we  wouldn't want  this on by
 * default
 *
 * M_SENDCONFIG for   modules to tell  fvwm that  they  want to  see each
 * module configuration command as   it is entered.  Causes  modconf.c to
 * look at each active module, find  the ones that sent M_SENDCONFIG, and
 * send a copy of the command in an M_CONFIG_INFO command.
 */

#define HEADER_SIZE         4
#define MAX_BODY_SIZE      (24)
#define MAX_PACKET_SIZE    (HEADER_SIZE+MAX_BODY_SIZE)

void KillModuleByName(char *name);
void AddToModList(char *tline);
void BroadcastMiniIcon(unsigned long event_type,
		       unsigned long data1, unsigned long data2,
		       unsigned long data3, unsigned long data4,
		       unsigned long data5, unsigned long data6,
		       unsigned long data7, unsigned long data8,
		       const char *name);

#endif /* MODULE_H */
