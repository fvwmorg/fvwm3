/* -*-c-*- */

#ifndef FVWM_MODULE_INTERFACE_H
#define FVWM_MODULE_INTERFACE_H

#include "Module.h"
#include "libs/queue.h"

/* please don't use msg_masks_t and PipeMask outside of module_interface.c.
 * They are only global to allow to access the IS_MESSAGE_SELECTED macro without
 * having to call a function. */
typedef struct msg_masks_t
{
	unsigned long m1;
	unsigned long m2;
} msg_masks_t;

/* module linked list record*/
typedef struct fmodule
{
	struct
	{
		unsigned is_cmdline_module;
	} flags;
	int readPipe;
	int writePipe;
	fqueue pipeQueue;
	msg_masks_t PipeMask;
	msg_masks_t NoGrabMask;
	msg_masks_t SyncMask;
	char *name;
	char *alias;
	struct fmodule *next;
} fmodule;

/*
 * I needed sendconfig off  to  identify open  pipes that want  config
 * info messages while active.
 * (...)
 * dje 10/2/98
 */

/* this is a bit long winded to allow MAX_MESSAGE to be 32 and not get an
 * integer overflow with (1 << MAX_MESSAGES) and even with
 * (1<<(MAX_MESSAGES-1)) - 1 */
#define DEFAULT_MASK   (MAX_MSG_MASK & ~(M_SENDCONFIG))
#define DEFAULT_XMASK  (DEFAULT_XMSG_MASK)

/*
 * Returns zero if the msg is not selected by the mask. Takes care of normal
 * and extended messages.
 */
#define IS_MESSAGE_IN_MASK(mask, msg) \
	(((msg)&M_EXTENDED_MSG) ? ((mask)->m2 & (msg)) : ((mask)->m1 & (msg)))

/*
 * Returns non zero if one of the specified messages is selected for the module
 */
#define IS_MESSAGE_SELECTED(module, msg_mask) \
	IS_MESSAGE_IN_MASK(&(module->PipeMask), (msg_mask))

/*
 * M_SENDCONFIG for   modules to tell  fvwm that  they  want to  see each
 * module configuration command as   it is entered.  Causes  modconf.c to
 * look at each active module, find  the ones that sent M_SENDCONFIG, and
 * send a copy of the command in an M_CONFIG_INFO command.
 */

/*
 *     module life-cycle handling
 */

/* initialize the system */
void initModules(void);
/* terminate the system */
void ClosePipes(void);
/* execute module */
fmodule *executeModuleDesperate(F_CMD_ARGS);
/* terminate module */
void KillModule(fmodule *module);
/* get the module placed after *prev, or the first if prev==NULL */
fmodule *module_get_next(fmodule *prev);
/* get the number of modules in memory - and hopefully in the list */
int countModules(void);

/*
 *     module communication functions
 */

/* Packet sending functions */
void BroadcastPacket(unsigned long event_type, unsigned long num_datum, ...);
void BroadcastConfig(unsigned long event_type, const FvwmWindow *t);
void BroadcastName(
	unsigned long event_type, unsigned long data1,
		   unsigned long data2, unsigned long data3, const char *name);
void BroadcastWindowIconNames(FvwmWindow *t, Bool window, Bool icon);
void BroadcastFvwmPicture(
	unsigned long event_type,
			  unsigned long data1, unsigned long data2,
			  unsigned long data3, FvwmPicture *picture, char *name);
void BroadcastPropertyChange(
	unsigned long argument, unsigned long data1,
			     unsigned long data2, char *string);
void BroadcastColorset(int n);
void BroadcastConfigInfoString(char *string);
void broadcast_xinerama_state(void);
void broadcast_ignore_modifiers(void);
void SendPacket(
	fmodule *module, unsigned long event_type, unsigned long num_datum,
	...);
void SendConfig(
	fmodule *module, unsigned long event_type, const FvwmWindow *t);
void SendName(
	fmodule *module, unsigned long event_type, unsigned long data1,
	      unsigned long data2, unsigned long data3, const char *name);
void PositiveWrite(fmodule *module, unsigned long *ptr, int size);
void FlushAllMessageQueues(void);
void FlushMessageQueue(fmodule *module);
void ExecuteCommandQueue(void);

/* packet receiving function */
Bool HandleModuleInput(Window w, fmodule *module, char *expect, Bool queue);

/* dead pipe signal handler */
RETSIGTYPE DeadPipe(int nonsense);

/*
 * exposed to be used by modconf.c
 */
char *skipModuleAliasToken(const char *string);
/* not used anywhere - it is here for modconf.c, right?*/
int is_message_selected(fmodule *module, unsigned long msg_mask);

#endif /* MODULE_H */
