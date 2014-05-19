/* -*-c-*- */

#ifndef FVWM_MODULE_LIST_H
#define FVWM_MODULE_LIST_H

#include "libs/Module.h"
#include "libs/fqueue.h"

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
		unsigned is_cmdline_module : 1;
        } xflags;
	int xreadPipe;
	int xwritePipe;
	fqueue xpipeQueue;
	msg_masks_t xPipeMask;
	msg_masks_t xNoGrabMask;
	msg_masks_t xSyncMask;
	char *xname;
	char *xalias;
} fmodule;

#define MOD_IS_CMDLINE(m) ((m)->xflags.is_cmdline_module)
#define MOD_SET_CMDLINE(m,on) ((m)->xflags.is_cmdline_module = !!(on))

typedef struct fmodule_store
{
	fmodule *module;
	struct fmodule_store *next;
} fmodule_store;

/* This defines the module list object */
typedef fmodule_store* fmodule_list;

/* this objects allows safe iteration over a module list */
typedef fmodule_store* fmodule_list_itr;

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

/* struct to store module input data */
typedef struct fmodule_input
{
	Window window;
	fmodule *module;
	char *command;
} fmodule_input;


/*
 *	Basic Module Handling Functions
 */

/* kill all modules */
void module_kill_all(void);

/* kill a module */
void module_kill(fmodule *module);

/* execute module wraper, desperate mode */
fmodule *executeModuleDesperate(F_CMD_ARGS);


/*
 *	Basic Module Communication Functions
 */

/* send "raw" data to the module */
/* module_send(fmodule *module, unsigned long *ptr, int size); */
void PositiveWrite(fmodule *module, unsigned long *ptr, int size);

/* returns a dynamicaly allocated struct with the received data
 * or NULL on error */
fmodule_input *module_receive(fmodule *module);

/* frees an input data struct */
void module_input_discard(fmodule_input *input);

/* returns true if received the "expect" string, false otherwise */
Bool module_input_expect(fmodule_input *input, char *expect);


/*
 *	Utility Functions
 */

/* initializes the given iterator */
void module_list_itr_init(fmodule_list_itr *itr);
/* gets the next module on the list */
fmodule *module_list_itr_next(fmodule_list_itr *itr);

/* free modules in the deathrow */
void module_cleanup(void);



/*
 *	Message Queue Handling Functions
 */

/* message queues */
void FlushAllMessageQueues(void);
void FlushMessageQueue(fmodule *module);


/*
 *	Misc Functions (should they be here?)
 */

/*
 * exposed to be used by modconf.c
 */
char *skipModuleAliasToken(const char *string);


/* dead pipe signal handler - empty */
RETSIGTYPE DeadPipe(int nonsense);

#endif /* MODULE_LIST_H */
