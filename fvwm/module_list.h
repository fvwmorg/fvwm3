/* -*-c-*- */

#ifndef FVWM_MODULE_LIST_H
#define FVWM_MODULE_LIST_H

#include "Module.h"
#include "libs/queue.h"

/* for F_CMD_ARGS */
#include "fvwm/fvwm.h"

/* please don't use msg_masks_t and PipeMask outside of module_interface.c.
 * They are only global to allow to access the IS_MESSAGE_SELECTED macro without
 * having to call a function. */
typedef struct msg_masks_t
{
	unsigned long m1;
	unsigned long m2;
} msg_masks_t;

/* module linked list record, only to be accessed by using the access macros
 * below */
typedef struct fmodule
{
	struct
	{
		unsigned is_cmdline_module;
	} xflags;
	int xreadPipe;
	int xwritePipe;
	fqueue xpipeQueue;
	msg_masks_t xPipeMask;
	msg_masks_t xNoGrabMask;
	msg_masks_t xSyncMask;
	char *xname;
	char *xalias;
	struct fmodule *xnext;
} fmodule;

/* struct to store module input data (in the near future) */
typedef struct fmodule_input
{
	Window window;
	fmodule *module;
	char *command;
} fmodule_input;

#define MOD_IS_CMDLINE(m) ((m)->xflags.is_cmdline_module)
#define MOD_SET_CMDLINE(m,on) ((m)->xflags.is_cmdline_module = !!(on))
#define MOD_READFD(m) ((m)->xreadPipe)
#define MOD_WRITEFD(m) ((m)->xwritePipe)
#define MOD_PIPEQUEUE(m) ((m)->xpipeQueue)
#define MOD_PIPEMASK(m) ((m)->xPipeMask)
#define MOD_NAME(m) ((m)->xname)
#define MOD_ALIAS(m) ((m)->xalias)

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
	IS_MESSAGE_IN_MASK(&(MOD_PIPEMASK(module)), (msg_mask))

/*
 * M_SENDCONFIG for   modules to tell  fvwm that  they  want to  see each
 * module configuration command as   it is entered.  Causes  modconf.c to
 * look at each active module, find  the ones that sent M_SENDCONFIG, and
 * send a copy of the command in an M_CONFIG_INFO command.
 */

/* kill all modules */
void module_kill_all(void);

/* kill a module */
void module_kill(fmodule *module);

/* execute a module - full function and wraper */
/*fmodule *module_execute(F_CMD_ARGS, Bool desperate, Bool do_listen_only)*/
fmodule *do_execute_module(F_CMD_ARGS, Bool desperate, Bool do_listen_only);
/* execute module wraper, desperate mode */
fmodule *executeModuleDesperate(F_CMD_ARGS);

/* send "raw" data to the module */
/* module_send(fmodule *module, unsigned long *ptr, int size); */
void PositiveWrite(fmodule *module, unsigned long *ptr, int size);

/*Bool module_receive(Window w, fmodule *module, char *expect, Bool queue);*/
Bool HandleModuleInput(Window w, fmodule *module, char *expect, Bool queue);

/* get the module placed after *prev, or the first if prev==NULL */
fmodule *module_get_next(fmodule *prev);

/* count the modules on list - not true for now. counts allocated modules */
int module_count(void);

/* message queues */
void FlushAllMessageQueues(void);
void FlushMessageQueue(fmodule *module);

/* dead pipe signal handler - empty */
RETSIGTYPE DeadPipe(int nonsense);

/*
 * exposed to be used by modconf.c
 */
char *skipModuleAliasToken(const char *string);

#endif /* MODULE_LIST_H */
