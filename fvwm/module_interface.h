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

#ifndef FVWM_MODULE_INTERFACE_H
#define FVWM_MODULE_INTERFACE_H

#include "Module.h"


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
/* this is a bit long winded to allow MAX_MESSAGE to be 32 and not get an
 * integer overflow with (1 << MAX_MESSAGES) and even with
 * (1<<(MAX_MESSAGES-1)) - 1 */
#define MAX_MASK   (MAX_MSG_MASK & ~(M_SENDCONFIG))
#define MAX_XMASK  (MAX_XMSG_MASK)


/*
 * M_SENDCONFIG for   modules to tell  fvwm that  they  want to  see each
 * module configuration command as   it is entered.  Causes  modconf.c to
 * look at each active module, find  the ones that sent M_SENDCONFIG, and
 * send a copy of the command in an M_CONFIG_INFO command.
 */

void initModules(void);
void ExecuteModuleCommand(Window w, int channel, char *command);
Bool HandleModuleInput(Window w, int channel, char *expect, Bool queue);
void KillModule(int channel);
void ClosePipes(void);
void BroadcastPacket(unsigned long event_type, unsigned long num_datum, ...);
void BroadcastConfig(unsigned long event_type, const FvwmWindow *t);
void BroadcastName(unsigned long event_type, unsigned long data1,
		   unsigned long data2, unsigned long data3, const char *name);
void BroadcastWindowIconNames(FvwmWindow *t, Bool window, Bool icon);
void BroadcastMiniIcon(unsigned long event_type,
		       unsigned long data1, unsigned long data2,
		       unsigned long data3, unsigned long data4,
		       unsigned long data5, unsigned long data6,
		       unsigned long data7, unsigned long data8,
		       const char *name);
void BroadcastColorset(int n);
void BroadcastConfigInfoString(char *string);
void broadcast_xinerama_state(void);
void broadcast_ignore_modifiers(void);

void SendPacket(int channel, unsigned long event_type,
                unsigned long num_datum, ...);
void SendConfig(int Module, unsigned long event_type, const FvwmWindow *t);
void SendName(int channel, unsigned long event_type, unsigned long data1,
	      unsigned long data2, unsigned long data3, const char *name);
void FlushAllMessageQueues(void);
void FlushMessageQueue(int Module);
void ExecuteCommandQueue(void);
void PositiveWrite(int module, unsigned long *ptr, int size);
RETSIGTYPE DeadPipe(int nonsense);
char *skipModuleAliasToken(const char *string);
int executeModuleDesperate(F_CMD_ARGS);
int is_message_selected(int module, unsigned long msg_mask);

#endif /* MODULE_H */
