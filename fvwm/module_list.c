/* -*-c-*- */
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include <errno.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

/* for time(); difftime() */
#include <time.h>

/* for F_CMD_ARGS */
#include "fvwm/fvwm.h"
#include "execcontext.h"
/* end of for CMD_ARGS */

/*for debug message*/
#include "fvwm.h"
#include "misc.h"
/* end of for debug message */

/* for get_current_read_file */
#include "read.h"
/* for busy cursor */
#include "cursor.h"

/* for Scr global */
#include "screen.h"

#include "externs.h"

/* for module syncronous */
#include "libs/ftime.h"
#include "libs/fvwmsignal.h"
#include "libs/log.h"
#include "libs/System.c"
#include "libs/envvar.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/wild.h"
#include "libs/fvwmsignal.h"
#include "libs/FEvent.h"
#include "events.h"
#include "bindings.h"

/* for positive write */

#include "module_list.h"
#include "module_interface.h"
/*
 * Use POSIX behaviour if we can, otherwise use SysV instead
 * should this be here?
 */
#ifndef O_NONBLOCK
#  define O_NONBLOCK  O_NDELAY
#endif

#ifndef FORK_CREATES_CHILD
#define FORK_CREATES_CHILD 0
#endif
#ifndef REMOVE_EXECUTABLE_EXTENSION
#define REMOVE_EXECUTABLE_EXTENSION 0
#endif

#define MOD_NOGRABMASK(m) ((m)->xNoGrabMask)
#define MOD_SYNCMASK(m) ((m)->xSyncMask)

typedef struct
{
	unsigned long *data;
	int size;
	int done;
} mqueue_object_type;


/* the linked list pointers to the first and last modules */
static fmodule_list module_list = NULL;
/* keep to-be-deleted modules in a deathrow until they are deleted safely. */
static fmodule_list death_row = NULL;

extern int moduleTimeout;

/*
 * static functions
 */
static fmodule *module_alloc(void);
static void module_free(fmodule *module);

/*
 * list handling functions
 */
static inline void module_list_insert(fmodule *module, fmodule_list *list);
static inline fmodule *module_list_remove(fmodule *module, fmodule_list *list);
static inline void module_list_destroy(fmodule_list *list);

static void KillModuleByName(char *name, char *alias);
static char *get_pipe_name(fmodule *module);
static void DeleteMessageQueueBuff(fmodule *module);

static inline void msg_mask_set(
	msg_masks_t *msg_mask, unsigned long m1, unsigned long m2);
static void set_message_mask(msg_masks_t *mask, unsigned long msg);


void module_kill_all(void)
{
	module_list_destroy(&module_list);

	return;
}

static fmodule *module_alloc(void)
{
	fmodule *module;

	module = fxmalloc(sizeof(fmodule));
	MOD_SET_CMDLINE(module, 0);
	MOD_READFD(module) = -1;
	MOD_WRITEFD(module) = -1;
	fqueue_init(&MOD_PIPEQUEUE(module));
	msg_mask_set(&MOD_PIPEMASK(module), DEFAULT_MASK, DEFAULT_MASK);
	msg_mask_set(&MOD_NOGRABMASK(module), 0, 0);
	msg_mask_set(&MOD_SYNCMASK(module), 0, 0);
	MOD_NAME(module) = NULL;
	MOD_ALIAS(module) = NULL;

	return module;
}

/* closes the pipes and frees every data associated with a module record */
static void module_free(fmodule *module)
{
	if (module == NULL)
	{
		return;
	}
	close(MOD_WRITEFD(module));
	close(MOD_READFD(module));

	if (MOD_NAME(module) != NULL)
	{
		free(MOD_NAME(module));
	}
	if (MOD_ALIAS(module) != NULL)
	{
		free(MOD_ALIAS(module));
	}
	while (!FQUEUE_IS_EMPTY(&(MOD_PIPEQUEUE(module))))
	{
		DeleteMessageQueueBuff(module);
	}
	free(module);

	return;
}

static inline void module_list_insert(fmodule *module, fmodule_list *list)
{
	fmodule_store *new_store;
	if (module == NULL)
	{
		return;
	}
	new_store = fxmalloc(sizeof(fmodule_store));
	new_store->module = module;
	new_store->next = *list;
	*list = new_store;

	return;
}

static inline fmodule *module_list_remove(fmodule *module, fmodule_list *list)
{
	fmodule_store **pos;

	if (module == NULL)
	{
		return NULL;
	}

	for (pos = list; *pos != NULL; pos = &((*pos)->next))
	{
		if ((*pos)->module == module)
		{
			/* found it */
			fmodule_store *current;

			current = *pos;
			*pos = (*pos)->next;
			free(current);

			return module;
		}
	}

	return NULL;
}

static inline void module_list_destroy(fmodule_list *list)
{
	fmodule_store *current;
	fmodule_store *next;

	for (current = *list; current != NULL; current = next)
	{
		next = current->next;
		module_free(current->module);
		free(current);
	}
	*list = NULL;

	return;
}

static fmodule *do_execute_module(
	F_CMD_ARGS, Bool desperate, Bool do_listen_only)
{
	int fvwm_to_app[2], app_to_fvwm[2];
	int i, val, nargs = 0;
	char *cptr = NULL;
	char **args = NULL;
	char *arg1 = NULL;
	char arg2[20];
	char arg3[20];
	char arg5[20];
	char arg6[20];
	char *token;
	extern char *ModulePath;
	Window win;
	FvwmWindow * const fw = exc->w.fw;
	fmodule *module;

	fvwm_to_app[0] = -1;
	fvwm_to_app[1] = -1;
	app_to_fvwm[1] = -1;
	app_to_fvwm[0] = -1;
	args = fxmalloc(7 * sizeof(char *));
	if (action == NULL)
	{
		goto err_exit;
	}
	if (fw)
	{
		win = FW_W(fw);
	}
	else
	{
		win = None;
	}
	action = GetNextToken(action, &cptr);
	if (!cptr)
	{
		goto err_exit;
	}
	arg1 = searchPath(ModulePath, cptr, EXECUTABLE_EXTENSION, X_OK);
	if (arg1 == NULL)
	{
		/* If this function is called in 'desparate' mode this means
		 * fvwm is trying a module name as a last resort.  In this case
		 * the error message is inappropriate because it was most
		 * likely a typo in a command, not a module name. */
		if (!desperate)
		{
			fvwm_debug(__func__,
				   "No such module '%s' in ModulePath '%s'",
				   cptr, ModulePath);
		}
		goto err_exit;
	}
	if (REMOVE_EXECUTABLE_EXTENSION && EXECUTABLE_EXTENSION != NULL)
	{
		char *p;
		char *ext = EXECUTABLE_EXTENSION;

		p = arg1 + strlen(arg1) - strlen(ext);
		if (strcmp(p, ext) == 0)
		{
			*p = 0;
		}
	}

	/* I want one-ended pipes, so I open two two-ended pipes,
	 * and close one end of each. I need one ended pipes so that
	 * I can detect when the module crashes/malfunctions */
	if (do_listen_only == True)
	{
		fvwm_to_app[0] = -1;
		fvwm_to_app[1] = -1;
	}
	else if (pipe(fvwm_to_app) != 0)
	{
		fvwm_debug(__func__, "Failed to open pipe");
		goto err_exit;
	}
	if (pipe(app_to_fvwm) != 0)
	{
		fvwm_debug(__func__, "Failed to open pipe2");
		goto err_exit;
	}
	if (
		fvwm_to_app[0] >= fvwmlib_max_fd ||
		fvwm_to_app[1] >= fvwmlib_max_fd ||
		app_to_fvwm[0] >= fvwmlib_max_fd ||
		app_to_fvwm[1] >= fvwmlib_max_fd)
	{
		fvwm_debug(__func__, "too many open fds");
		goto err_exit;
	}

	/* all ok, create the space and fill up */
	module = module_alloc();

	MOD_NAME(module) = stripcpy(cptr);
	free(cptr);
	snprintf(arg2, sizeof(arg2), "%d", app_to_fvwm[1]);
	snprintf(arg3, sizeof(arg3), "%d", fvwm_to_app[0]);
	snprintf(arg5, sizeof(arg5), "%lx", (unsigned long)win);
	snprintf(arg6, sizeof(arg6), "%lx", (unsigned long)exc->w.wcontext);
	args[0] = arg1;
	args[1] = arg2;
	args[2] = arg3;
	args[3] = (char *)get_current_read_file();
	if (!args[3])
	{
		args[3] = "none";
	}
	args[4] = arg5;
	args[5] = arg6;
	for (nargs = 6; action = GetNextToken(action, &token), token; nargs++)
	{
		args = fxrealloc((void *)args, (nargs + 2), sizeof(char *));
		args[nargs] = token;
		if (MOD_ALIAS(module) == NULL)
		{
			const char *ptr = skipModuleAliasToken(args[nargs]);

			if (ptr && *ptr == '\0')
			{
				MOD_ALIAS(module) = stripcpy(args[nargs]);
			}
		}
	}
	args[nargs] = NULL;

	/* Try vfork instead of fork. The man page says that vfork is better!
	 */
	/* Also, had to change exit to _exit() */
	/* Not everyone has vfork! */
	val = fork();
	if (val > 0)
	{
		/* This fork remains running fvwm */
		/* close appropriate descriptors from each pipe so
		 * that fvwm will be able to tell when the app dies */
		close(app_to_fvwm[1]);
		/* dont't care that this may be -1 */
		close(fvwm_to_app[0]);

		/* add these pipes to fvwm's active pipe list */
		MOD_WRITEFD(module) = fvwm_to_app[1];
		MOD_READFD(module) = app_to_fvwm[0];
		msg_mask_set(
			&MOD_PIPEMASK(module), DEFAULT_MASK, DEFAULT_MASK);
		if (DoingCommandLine)
		{
			/* add to the list of command line modules */
			MOD_SET_CMDLINE(module, 1);
		}

		/* make the PositiveWrite pipe non-blocking. Don't want to jam
		 * up fvwm because of an uncooperative module */
		if (MOD_WRITEFD(module) >= 0)
		{
			fcntl(MOD_WRITEFD(module), F_SETFL, O_NONBLOCK);
		}
		/* Mark the pipes close-on exec so other programs
		 * won`t inherit them */
		if (fcntl(MOD_READFD(module), F_SETFD, 1) == -1)
		{
			fvwm_debug(__func__,
				   "module close-on-exec failed");
		}
		if (
			MOD_WRITEFD(module) >= 0 &&
			fcntl(MOD_WRITEFD(module), F_SETFD, 1) == -1)
		{
			fvwm_debug(__func__,
				   "module close-on-exec failed");
		}
		/* module struct is completed, insert into the list */
		module_list_insert(module, &module_list);

	}
	else if (val == 0)
	{
		/* this is the child */
		/* this fork execs the module */
		if (FORK_CREATES_CHILD)
		{
			/* dont't care that this may be -1 */
			close(fvwm_to_app[1]);
			close(app_to_fvwm[0]);
		}
		fvmm_deinstall_signals();
		if (!Pdefault)
		{
			char visualid[32];
			char colormap[32];

			snprintf(visualid, sizeof(visualid),
				"FVWM_VISUALID=%lx",
				XVisualIDFromVisual(Pvisual));
			flib_putenv("FVWM_VISUALID", visualid);
			snprintf(colormap, sizeof(colormap),
				"FVWM_COLORMAP=%lx", Pcmap);
			flib_putenv("FVWM_COLORMAP", colormap);
		}
		else
		{
			flib_unsetenv("FVWM_VISUALID");
			flib_unsetenv("FVWM_COLORMAP");
		}

		/* Why is this execvp??  We've already searched the module
		 * path! */
		execvp(arg1, args);
		fvwm_debug(__func__, "Execution of module failed: %s", arg1);
		perror("");
		close(app_to_fvwm[1]);
		if (fvwm_to_app[0] >= 0)
		{
			close(fvwm_to_app[0]);
		}
		if (FORK_CREATES_CHILD)
		{
			exit(1);
		}
	}
	else
	{
		fvwm_debug(__func__, "Fork failed");
		module_free(module);
		module = NULL;
	}
	free(arg1);
	for (i = 6; i < nargs; i++)
	{
		if (args[i] != 0)
		{
			free(args[i]);
		}
	}
	free(args);

	return module;

  err_exit:
	if (arg1 != NULL)
	{
		free(arg1);
	}
	if (cptr != NULL)
	{
		free(cptr);
	}
	if (args != NULL)
	{
		free(args);
	}
	if (fvwm_to_app[0] >= 0)
	{
		close(fvwm_to_app[0]);
	}
	if (fvwm_to_app[1] >= 0)
	{
		close(fvwm_to_app[1]);
	}
	if (app_to_fvwm[0] >= 0)
	{
		close(app_to_fvwm[0]);
	}
	if (app_to_fvwm[1] >= 0)
	{
		close(app_to_fvwm[1]);
	}

	return NULL;
}

fmodule *executeModuleDesperate(F_CMD_ARGS)
{
	fmodule *m;

	m = do_execute_module(F_PASS_ARGS, True, False);

	return m;
}

void module_kill(fmodule *module)
{
	module_list_insert(
		module_list_remove(module, &module_list), &death_row);

	return;
}

/* free modules in the deathrow */
void module_cleanup(void)
{
	module_list_destroy(&death_row);

	return;
}

/* void module_send(fmodule *module, unsigned long *ptr, int size) */
/* This used to be marked "fvwm_inline".  I removed this
   when I added the lockonsend logic.  The routine seems too big to
   want to inline.  dje 9/4/98 */
extern int myxgrabcount;                /* defined in libs/Grab.c */
extern char *ModuleUnlock;              /* defined in libs/Module.c */
void PositiveWrite(fmodule *module, unsigned long *ptr, int size)
{
	msg_masks_t mask;

	if (ptr == NULL)
	{
		return;
	}
	if (MOD_WRITEFD(module) == -1)
	{
		return;
	}
	if (!IS_MESSAGE_IN_MASK(&(MOD_PIPEMASK(module)), ptr[1]))
	{
		return;
	}

	/* a dirty hack to prevent FvwmAnimate triggering during Recapture */
	/* would be better to send RecaptureStart and RecaptureEnd messages. */
	/* If module is lock on send for iconify message and it's an
	 * iconify event and server grabbed, then return */
	mask.m1 = (MOD_NOGRABMASK(module).m1 & MOD_SYNCMASK(module).m1);
	mask.m2 = (MOD_NOGRABMASK(module).m2 & MOD_SYNCMASK(module).m2);
	if (IS_MESSAGE_IN_MASK(&mask, ptr[1]) && myxgrabcount != 0)
	{
		return;
	}

	/* DV: This was once the AddToMessageQueue function.  Since it was only
	 * called once, put it in here for better performance. */
	{
		mqueue_object_type *c;

		c = (mqueue_object_type *)malloc(
			sizeof(mqueue_object_type) + size);
		if (c == NULL)
		{
			fvwm_debug(__func__, "malloc failed\n");
			exit(1);
		}
		c->size = size;
		c->done = 0;
		c->data = (unsigned long *)(c + 1);
		memcpy((void*)c->data, (const void*)ptr, size);
		fqueue_add_at_end(&(MOD_PIPEQUEUE(module)), c);
	}

	/* dje, from afterstep, for FvwmAnimate, allows modules to sync with
	 * fvwm. this is disabled when the server is grabbed, otherwise
	 * deadlocks happen. M_LOCKONSEND has been replaced by a separated
	 * mask which defines on which messages the fvwm-to-module
	 * communication need to be lock on send. olicha Nov 13, 1999 */
	/* migo (19-Aug-2000): removed !myxgrabcount to sync M_DESTROY_WINDOW
	 */
	/* dv (06-Jul-2002): added the !myxgrabcount again.  Deadlocks *do*
	 * happen without it.  There must be another way to fix
	 * M_DESTROY_WINDOW handling in FvwmEvent. */
	/*if (IS_MESSAGE_IN_MASK(&(MOD_SYNCMASK(module)), ptr[1]))*/
	if (
		IS_MESSAGE_IN_MASK(
			&(MOD_SYNCMASK(module)), ptr[1]) && !myxgrabcount)
	{
		fd_set readSet;
		int channel = MOD_READFD(module);
		struct timeval timeout;
		Bool done = False;
		fmodule_input *input;

		FlushMessageQueue(module);

		while (!done)
		{
			int rc = 0;
			/*
			 * We give the read a long timeout; if the module
			 * fails to respond within this time then it deserves
			 * to be KILLED!
			 *
			 * NOTE: rather than impose an arbitrary timeout on the
			 * user, we will make this a configuration parameter.
			 */
			do
			{
				timeout.tv_sec = moduleTimeout;
				timeout.tv_usec = 0;
				FD_ZERO(&readSet);
				FD_SET(channel, &readSet);

				/* Wait for input to arrive on just one
				 * descriptor, with a timeout (fvwmSelect <= 0)
				 * or read() returning wrong size is bad news
				 */
				rc = fvwmSelect(
					channel + 1, &readSet, NULL, NULL,
					&timeout);
				/* retry if select() failed with EINTR */
			} while (rc < 0 && !isTerminated && (errno == EINTR));

			if ( isTerminated )
			{
				break;
			}

			if (rc > 0)
			{
				input = module_receive(module);
				if (
					input == NULL ||
					module_input_expect(input,
						ModuleUnlockResponse))
				{
					module_input_discard(input);
					done = True;
				}
				else
				{
					module_input_execute(input);
				}

			}
			else
			{
				char *name;

				name = get_pipe_name(module);
				/* Doh! Something has gone wrong - get rid of
				 * the offender! */
				fvwm_debug(__func__,
					   "Failed to read descriptor from"
					   " '%s':\n"
					   "- data available=%c\n"
					   "- terminate signal=%c\n",
					   name,
					   (FD_ISSET(channel, &readSet) ?
					    'Y' : 'N'),
					   isTerminated ? 'Y' : 'N');
				module_kill(module);
				break;
			}


			/* Execute all messages from the module until UNLOCK is
			 * received N.B. This may cause recursion if a command
			 * results in a sync message to another module, which
			 * in turn may send a command that results in another
			 * sync message to this module.
			 * Hippo: I don't think this will cause deadlocks, but
			 * the third time we get here the first times UNLOCK
			 * will be read and then on returning up the third
			 * level UNLOCK will be read at the first level. This
			 * could be difficult to fix without turning queueing
			 * on.  Turning queueing on may be bad because it can
			 * be useful for modules to be able to inject commands
			 * from modules in a synchronous manner. e.g.
			 * FvwmIconMan can tell FvwmAnimate to do an animation
			 * when a window is de-iconified from the IconMan,
			 * queueing make s this happen too late. */
		}
	}

	return;
}

fmodule_input *module_receive(fmodule *module)
{
	unsigned long size;
	unsigned long cont;
	Window win;
	int n;
	fmodule_input *input = NULL;

	n = read(MOD_READFD(module), &win, sizeof(Window));
	if (n < sizeof(Window))
	{
		/* exit silently, module should have died.. */
		goto err;
	}

	n = read(MOD_READFD(module), &size, sizeof(size));
	if (n < sizeof(size))
	{
		fvwm_debug(__func__,
			   "Fail to read command size (Module: %p, read: %i, "
			   "size: %i)", module, n, (int)sizeof(size));
		goto err;
	}

	if (size > MAX_MODULE_INPUT_TEXT_LEN)
	{
		fvwm_debug(__func__,
			   "Module(%p) command is too big (%ld), limit is %d",
			   module, size, MAX_MODULE_INPUT_TEXT_LEN);
		/* The rest of the output from this module is going to be
		 * scrambled so let's kill it rather than risk interpreting
		 * garbage */
		goto err;
	}

	/* allocate all storage at once */
	/* also save space for the '\0' termination character */
	input = fxmalloc(sizeof(fmodule_input) + sizeof(char)*(size + 1));

	input->module = module;
	input->window = win;
	input->command = (char*)input + sizeof(fmodule_input);

	n = read(MOD_READFD(module), input->command, size);
	if (n < size)
	{
		fvwm_debug(__func__,
			   "Fail to read command (Module: %p, read: %i, size:"
			   " %ld)", module, n, size);
		goto err;
	}
	input->command[n] = '\0';
	n = read(MOD_READFD(module), &cont, sizeof(cont));
	if (n < sizeof(cont))
	{
		fvwm_debug(__func__,
			   "Module %p, Size Problems (read: %d, size: %d)",
			   module, n, (int)sizeof(cont));
		goto err;
	}
	if (cont == 0)
	{
		/* this is documented as a valid way for a module to quit
		 * so let's not complain */
		module_kill(module);
	}

	return input;
err:
	module_kill(module);
	module_input_discard(input);
	return NULL;
}

/* frees the module input data struct */
void module_input_discard(fmodule_input *input)
{
	if (input==NULL)
	{
		return;
	}
	free(input);
}

/* returns true if the module command matches "expect", false otherwise */
Bool module_input_expect(fmodule_input *input, char *expect)
{
	if (input == NULL || input->command == NULL || expect == NULL)
	{
		return False;
	}

	if (strncasecmp(input->command, expect, strlen(expect))	== 0)
	{
		/* the module sent the expected string */
		return True;
	}

	return False;
}

void module_list_itr_init(fmodule_list_itr *itr)
{
	*itr = module_list;

	return;
}

fmodule *module_list_itr_next(fmodule_list_itr *itr)
{
	fmodule *module;

	if (*itr == NULL)
	{
		return NULL;
	}
	module = (*itr)->module;
	*itr = (*itr)->next;

	return module;
}

static void KillModuleByName(char *name, char *alias)
{
	fmodule_list_itr moditr;
	fmodule *module;

	if (name == NULL)
	{
		return;
	}
	module_list_itr_init(&moditr);
	while ( (module = module_list_itr_next(&moditr)) != NULL)
	{
		if (
			MOD_NAME(module) != NULL &&
			matchWildcards(name, MOD_NAME(module)) &&
			(!alias || (
				 MOD_ALIAS(module) &&
				 matchWildcards(alias, MOD_ALIAS(module)))))
		{
			module_kill(module);
		}
	}

	return;
}

static char *get_pipe_name(fmodule *module)
{
	char *name;

	if (MOD_NAME(module) != NULL)
	{
		if (MOD_ALIAS(module) != NULL)
		{
			xasprintf(&name, "%s %s", MOD_NAME(module),
			    MOD_ALIAS(module));
		}
		else
		{
			xasprintf(&name, "%s", MOD_NAME(module));
		}
	}
	else
	{
		name = fxstrdup("(null)");
	}

	return name;
}

/*
 * returns a pointer inside a string (just after the alias) if ok or NULL
 */
char *skipModuleAliasToken(const char *string)
{
#define is_valid_first_alias_char(ch) (isalpha(ch) || (ch) == '/')
#define is_valid_alias_char(ch) (is_valid_first_alias_char(ch)  \
				|| isalnum(ch) || (ch) == '-' || \
				(ch) == '.' || (ch) == '/')

	if (is_valid_first_alias_char(*string))
	{
		int len = 1;
		string++;
		while (*string && is_valid_alias_char(*string))
		{
			if (++len > MAX_MODULE_ALIAS_LEN)
			{
				return NULL;
			}
			string++;
		}
		return (char *)string;
	}

	return NULL;
#undef is_valid_first_alias_char
#undef is_valid_alias_char
}

/* message mask handling - does this belong here? */

static inline void msg_mask_set(
	msg_masks_t *msg_mask, unsigned long m1, unsigned long m2)
{
	msg_mask->m1 = m1;
	msg_mask->m2 = m2;

	return;
}

/*
 * Sets the mask to the specific value.  If M_EXTENDED_MSG is set in mask, the
 * function operates only on the extended messages, otherwise it operates only
 * on normal messages.
 */
static void set_message_mask(msg_masks_t *mask, unsigned long msg)
{
	if (msg & M_EXTENDED_MSG)
	{
		mask->m2 = (msg & ~M_EXTENDED_MSG);
	}
	else
	{
		mask->m1 = msg;
	}

	return;
}


/* message queues */

static void DeleteMessageQueueBuff(fmodule *module)
{
	mqueue_object_type *obj;

	if (fqueue_get_first(&(MOD_PIPEQUEUE(module)), (void **)&obj) == 1)
	{
		/* remove from queue */
		fqueue_remove_or_operate_from_front(
			&(MOD_PIPEQUEUE(module)), NULL, NULL, NULL, NULL);
		/* we don't need to free the obj->data here because it's in the
		 * same malloced block as the obj itself. */
		free(obj);
	}

	return;
}

void FlushMessageQueue(fmodule *module)
{
	mqueue_object_type *obj;
	char *dptr;
	int a;

	if (module == NULL)
	{
		return;
	}

	while (fqueue_get_first(&(MOD_PIPEQUEUE(module)), (void **)&obj) == 1)
	{
		dptr = (char *)obj->data;
		while (obj->done < obj->size)
		{
			a = write(MOD_WRITEFD(module), &dptr[obj->done],
				  obj->size - obj->done);
			if (a >=0)
			{
				obj->done += a;
			}
			/* the write returns EWOULDBLOCK or EAGAIN if the pipe
			 * is full. (This is non-blocking I/O). SunOS returns
			 * EWOULDBLOCK, OSF/1 returns EAGAIN under these
			 * conditions. Hopefully other OSes return one of these
			 * values too. Solaris 2 doesn't seem to have a man
			 * page for write(2) (!) */
			else if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				fd_set writeSet;
				struct timeval timeout;
				int channel = MOD_WRITEFD(module);
				int rc = 0;

				do
				{
					/* Wait until the pipe accepts further
					 * input */
					timeout.tv_sec = moduleTimeout;
					timeout.tv_usec = 0;
					FD_ZERO(&writeSet);
					FD_SET(channel, &writeSet);
					rc = fvwmSelect(
						channel + 1, NULL, &writeSet,
						NULL, &timeout);
					/* retry if select() failed with EINTR
					 */
				} while ((rc < 0) && !isTerminated &&
					 (errno == EINTR));

				if ( isTerminated )
				{
					return;
				}
				if (!FD_ISSET(channel, &writeSet))
				{
					char *name;

					name = get_pipe_name(module);
					/* Doh! Something has gone wrong - get
					 * rid of the offender! */
					fvwm_debug(__func__,
						   "Failed to write descriptor to"
						   " '%s':\n"
						   "- select rc=%d\n"
						   "- terminate signal=%c\n",
						   name, rc, isTerminated ?
						   'Y' : 'N');
					module_kill(module);
					free(name);

					return;
				}

				/* pipe accepts further input; continue */
				continue;
			}
			else if (errno != EINTR)
			{
				module_kill(module);

				return;
			}
		}
		DeleteMessageQueueBuff(module);
	}

	return;
}

void FlushAllMessageQueues(void)
{
	fmodule_list_itr moditr;
	fmodule *module;

	module_list_itr_init(&moditr);
	while ( (module = module_list_itr_next(&moditr)) != NULL)
	{
		FlushMessageQueue(module);
	}

	return;
}

/* empty, only here so that the signal handling initialization code is the
 * same for modules and fvwm  */
RETSIGTYPE DeadPipe(int sig)
{
	SIGNAL_RETURN;
}

void CMD_Module(F_CMD_ARGS)
{
	do_execute_module(F_PASS_ARGS, False, False);

	return;
}

void CMD_ModuleListenOnly(F_CMD_ARGS)
{
	do_execute_module(F_PASS_ARGS, False, True);

	return;
}

void CMD_KillModule(F_CMD_ARGS)
{
	char *name;
	char *alias = NULL;

	action = GetNextToken(action,&name);
	if (!name)
	{
		return;
	}

	GetNextToken(action, &alias);
	KillModuleByName(name, alias);
	free(name);
	if (alias)
	{
		free(alias);
	}
	return;
}

void CMD_ModuleSynchronous(F_CMD_ARGS)
{
	int sec = 0;
	char *next;
	char *token;
	char *expect = fxstrdup(ModuleFinishedStartupResponse);
	fmodule *module;
	fd_set in_fdset;
	fd_set out_fdset;
	time_t start_time;
	Bool done = False;
	Bool need_ungrab = False;
	char *escape = NULL;
	XEvent tmpevent;

	if (!action)
	{
		return;
	}

	token = PeekToken(action, &next);
	if (StrEquals(token, "expect"))
	{
		token = PeekToken(next, &next);
		if (token)
		{
			expect = fxstrdup(token);
		}
		action = next;
		token = PeekToken(action, &next);
	}
	if (token && StrEquals(token, "timeout"))
	{
		if (GetIntegerArguments(next, &next, &sec, 1) > 0 && sec > 0)
		{
			/* we have a delay, skip the number */
			action = next;
		}
		else
		{
			fvwm_debug(__func__, "illegal timeout");
			goto done;
		}
	}

	if (!action)
	{
		/* no module name */
		goto done;
	}

	module = do_execute_module(F_PASS_ARGS, False, False);
	if (module == NULL)
	{
		/* executing the module failed, just return */
		goto done;
	}

	/* Busy cursor stuff */
	if (Scr.BusyCursor & BUSY_MODULESYNCHRONOUS)
	{
		if (GrabEm(CRS_WAIT, GRAB_BUSY))
			need_ungrab = True;
	}

	/* wait for module input */
	start_time = time(NULL);

	while (!done)
	{
		struct timeval timeout;
		int num_fd;

		/* A signal here could interrupt the select call. We would
		 * then need to restart our select, unless the signal was
		 * a "terminate" signal. Note that we need to reinitialise
		 * all of select's parameters after it has returned. */
		do
		{
			FD_ZERO(&in_fdset);
			FD_ZERO(&out_fdset);

			FD_SET(MOD_READFD(module), &in_fdset);

			if (!FQUEUE_IS_EMPTY(&MOD_PIPEQUEUE(module)))
			{
				FD_SET(MOD_WRITEFD(module), &out_fdset);
			}
			timeout.tv_sec = 0;
			timeout.tv_usec = 1;
			num_fd = fvwmSelect(
				fvwmlib_max_fd, &in_fdset, &out_fdset, 0,
				&timeout);
		} while (num_fd < 0 && !isTerminated);

		/* Exit if we have received a "terminate" signal */
		if (isTerminated)
		{
			break;
		}

		if (num_fd > 0)
		{
			if (FD_ISSET(MOD_READFD(module), &in_fdset))
			{
				fmodule_input * input;

				/* Check for module input. */
				input = module_receive(module);
				if (
					input == NULL ||
					module_input_expect(input,expect))
				{
					module_input_discard(input);
					done = True;
				}
				else
				{
					module_input_execute(input);
				}

			}

			if ((MOD_WRITEFD(module) >= 0) &&
			    FD_ISSET(MOD_WRITEFD(module), &out_fdset))
			{
				FlushMessageQueue(module);
			}
		}

		usleep(1000);
		if (difftime(time(NULL), start_time) >= sec && sec)
		{
			/* timeout */
			done = True;
		}

		/* Check for "escape function" */
		if (FPending(dpy) &&
		    FCheckMaskEvent(dpy, KeyPressMask, &tmpevent))
		{
			int context;
			XClassHint *class;
			char *name;
			Window w;

			context = GetContext(
				NULL, exc->w.fw, &tmpevent, &w);
			if (exc->w.fw != NULL)
			{
				class = &(exc->w.fw->class);
				name = exc->w.fw->name.name;
			}
			else
			{
				class = NULL;
				name = NULL;
			}
			escape = CheckBinding(
				Scr.AllBindings,
				tmpevent.xkey.keycode, tmpevent.xkey.state,
				GetUnusedModifiers(), context, BIND_KEYPRESS,
				class, name);
			if (escape != NULL)
			{
				if (!strcasecmp(escape,"escapefunc"))
				{
					done = True;
				}
			}
		}
	} /* while */

	if (need_ungrab)
	{
		UngrabEm(GRAB_BUSY);
	}
done:
	free(expect);
}

/* mask handling - does this belong here? */

void CMD_set_mask(F_CMD_ARGS)
{
	unsigned long val;

	if (exc->m.module == NULL)
	{
		return;
	}
	if (!action || sscanf(action, "%lu", &val) != 1)
	{
		val = 0;
	}
	set_message_mask(&(MOD_PIPEMASK(exc->m.module)), (unsigned long)val);

	return;
}

void CMD_set_sync_mask(F_CMD_ARGS)
{
	unsigned long val;

	if (exc->m.module == NULL)
	{
		return;
	}
	if (!action || sscanf(action,"%lu",&val) != 1)
	{
		val = 0;
	}
	set_message_mask(&(MOD_SYNCMASK(exc->m.module)), (unsigned long)val);

	return;
}

void CMD_set_nograb_mask(F_CMD_ARGS)
{
	unsigned long val;

	if (exc->m.module == NULL)
	{
		return;
	}
	if (!action || sscanf(action,"%lu",&val) != 1)
	{
		val = 0;
	}
	set_message_mask(&(MOD_NOGRABMASK(exc->m.module)), (unsigned long)val);

	return;
}
