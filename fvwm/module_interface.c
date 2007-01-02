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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 */

/*
 *
 * code for launching fvwm modules.
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "libs/ftime.h"
#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "read.h"
#include "events.h"
#include "geometry.h"
#include "fvwmsignal.h"
#include "decorations.h"
#include "commands.h"

/*
 * Use POSIX behaviour if we can, otherwise use SysV instead
 */
#ifndef O_NONBLOCK
#  define O_NONBLOCK  O_NDELAY
#endif

/* the linked list pointers to the first and last modules */
static fmodule *module_list;

typedef struct
{
	unsigned long *data;
	int size;
	int done;
} mqueue_object_type;

static const unsigned long dummy = 0;

static void DeleteMessageQueueBuff(fmodule *module);
static void AddToCommandQueue(Window w, fmodule *module, char * command);

static int num_modules = 0;

static inline void msg_mask_set(
	msg_masks_t *msg_mask, unsigned long m1, unsigned long m2)
{
	msg_mask->m1 = m1;
	msg_mask->m2 = m2;

	return;
}

static fmodule *module_alloc(void)
{
	fmodule *module;

	num_modules++;
	module = (fmodule *)safemalloc(sizeof(fmodule));
	module->flags.is_cmdline_module = 0;
	module->readPipe = -1;
	module->writePipe = -1;
	fqueue_init(&module->pipeQueue);
	msg_mask_set(&module->PipeMask, DEFAULT_MASK, DEFAULT_MASK);
	msg_mask_set(&module->NoGrabMask, 0, 0);
	msg_mask_set(&module->SyncMask, 0, 0);
	module->name = NULL;
	module->alias = NULL;
	module->next = NULL;

	return module;
}

static inline void module_insert(fmodule *module)
{
	module->next = module_list;
	module_list = module;

	return;
}

static inline void module_remove(fmodule *module)
{
	if (module == NULL)
	{
		return;
	}
	if (module == module_list)
	{
		DBUG("module_remove", "Removing from module list");
		module_list = module->next;
	}
	else
	{
		fmodule *parent;
		fmodule *current;

		/* find it*/
		for (
			current = module_list->next, parent = module_list;
			current != NULL;
			parent = current, current = current->next)
		{
			if (current == module)
			{
				break;
			}
		}
		/* remove from the list */
		if (current != NULL)
		{
			DBUG("module_remove", "Removing from module list");
			parent->next = module->next;
		}
		else
		{
			fvwm_msg(
				ERR, "module_remove",
				"Tried to remove a not listed module!");

			return;
		}
	}
}


/* closes the pipes and frees every data associated with a module record */
static void module_free(fmodule *module)
{
	if (module == NULL)
	{
		return;
	}
	if (module->writePipe >= 0)
	{
		close(module->writePipe);
	}
	if (module->readPipe >= 0)
	{
		close(module->readPipe);
	}
	if (module->name != NULL)
	{
		free(module->name);
	}
	if (module->alias != NULL)
	{
		free(module->alias);
	}
	while (!FQUEUE_IS_EMPTY(&(module->pipeQueue)))
	{
		DeleteMessageQueueBuff(module);
	}
	num_modules--;
	free(module);

	return;
}

fmodule *module_get_next(fmodule *prev)
{
	if (prev == NULL)
	{
		return module_list;
	}
	else
	{
		return prev->next;
	}
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

static char *get_pipe_name(fmodule *module)
{
	char *name="";

	if (module->name != NULL)
	{
		if (module->alias != NULL)
		{
			name = CatString3(
				module->name, " ", module->alias);
		}
		else
		{
			name = module->name;
		}
	}
	else
	{
		name = CatString3("(null)", "", "");
	}

	return name;
}

void module_add_to_fdsets(fmodule *module, fd_set *in, fd_set *out)
{
	if (module == NULL)
	{
		return;
	}
	if (
		(module->readPipe > GetFdWidth()) ||
		(module->writePipe > GetFdWidth())
	)
	{
		fvwm_msg(ERR, "module_add_to_fdsets", "Too many fds open!");
		/* kill it now, otherwise it will never be killed */
		KillModule(module);
		return;
	}

	if (module->readPipe >= 0)
	{
		FD_SET(module->readPipe, in);
	}
	if (!FQUEUE_IS_EMPTY(&(module->pipeQueue)))
	{
		FD_SET(module->writePipe, out);
	}
	return;
}

void initModules(void)
{
	DBUG("initModules", "initializing the module list header");
	/* the list is empty */
	module_list = NULL;

	return;
}

void ClosePipes(void)
{
	fmodule *module;

	/*
	 * this improves speed, but having a single remotion routine should
	 * help in mainainability.. replace by module_remove calls?
	 */
	for (module = module_list; module != NULL; module = module->next)
		{
		module_free(module);
	}
	module_list = NULL;

	return;
}

static fmodule *do_execute_module(
	F_CMD_ARGS, Bool desperate, Bool do_listen_only)
{
	int fvwm_to_app[2], app_to_fvwm[2];
	int i, val, nargs = 0;
	char *cptr;
	char **args;
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

	args = (char **)safemalloc(7 * sizeof(char *));

	/* Olivier: Why ? */
	/*   if (eventp->type != KeyPress) */
	/*     UngrabEm(); */

	if (action == NULL)
	{
		free(args);

		return NULL;
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
		free(args);

		return NULL;
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
			fvwm_msg(
				ERR, "executeModule",
				"No such module '%s' in ModulePath '%s'",
				cptr, ModulePath);
		}
		free(args);
		free(cptr);

		return NULL;
	}

#ifdef REMOVE_EXECUTABLE_EXTENSION
	{
		char *p;

		p = arg1 + strlen(arg1) - strlen(EXECUTABLE_EXTENSION);
		if (strcmp(p, EXECUTABLE_EXTENSION) == 0)
		{
			*p = 0;
		}
	}
#endif

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
		fvwm_msg(ERR, "executeModule", "Failed to open pipe");
		free(arg1);
		free(cptr);
		free(args);

		return NULL;
	}
	if (pipe(app_to_fvwm)!=0)
	{
		fvwm_msg(ERR, "executeModule", "Failed to open pipe2");
		free(arg1);
		free(cptr);
		/* dont't care that these may be -1 */
		close(fvwm_to_app[0]);
		close(fvwm_to_app[1]);
		free(args);

		return NULL;
	}

	/* all ok, create the space and insert into the list */
	module = module_alloc();
	module_insert(module);

	module->name = stripcpy(cptr);
	free(cptr);
	sprintf(arg2, "%d", app_to_fvwm[1]);
	sprintf(arg3, "%d", fvwm_to_app[0]);
	sprintf(arg5, "%lx", (unsigned long)win);
	sprintf(arg6, "%lx", (unsigned long)exc->w.wcontext);
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
		args = (char **)saferealloc(
			(void *)args, (nargs + 2) * sizeof(char *));
		args[nargs] = token;
		if (module->alias == NULL)
		{
			const char *ptr = skipModuleAliasToken(args[nargs]);

			if (ptr && *ptr == '\0')
			{
				module->alias = stripcpy(args[nargs]);
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
		module->writePipe = fvwm_to_app[1];
		module->readPipe = app_to_fvwm[0];
		msg_mask_set(&module->PipeMask, DEFAULT_MASK, DEFAULT_MASK);
		free(arg1);
		if (DoingCommandLine)
		{
			/* add to the list of command line modules */
			DBUG("executeModule", "starting commandline module\n");
			module->flags.is_cmdline_module = 1;
		}

		/* make the PositiveWrite pipe non-blocking. Don't want to jam
		 * up fvwm because of an uncooperative module */
		if (module->writePipe >= 0)
		{
			fcntl(module->writePipe, F_SETFL, O_NONBLOCK);
		}
		/* Mark the pipes close-on exec so other programs
		 * won`t inherit them */
		if (fcntl(module->readPipe, F_SETFD, 1) == -1)
		{
			fvwm_msg(
				ERR, "executeModule",
				"module close-on-exec failed");
		}
		if (
			module->writePipe >= 0 &&
			fcntl(module->writePipe, F_SETFD, 1) == -1)
		{
			fvwm_msg(
				ERR, "executeModule",
				"module close-on-exec failed");
		}
		for (i = 6; i < nargs; i++)
		{
			if (args[i] != 0)
			{
				free(args[i]);
			}
		}
	}
	else if (val ==0)
	{
		/* this is the child */
		/* this fork execs the module */
#ifdef FORK_CREATES_CHILD
		/* dont't care that this may be -1 */
		close(fvwm_to_app[1]);
		close(app_to_fvwm[0]);
#endif
		if (!Pdefault)
		{
			char visualid[32];
			char colormap[32];

			sprintf(
				visualid, "FVWM_VISUALID=%lx",
				XVisualIDFromVisual(Pvisual));
			flib_putenv("FVWM_VISUALID", visualid);
			sprintf(colormap, "FVWM_COLORMAP=%lx", Pcmap);
			flib_putenv("FVWM_COLORMAP", colormap);
		}
		else
		{
			flib_unsetenv("FVWM_VISUALID");
			flib_unsetenv("FVWM_COLORMAP");
		}

		/* Why is this execvp??  We've already searched the module
		 * path! */
		execvp(arg1,args);
		fvwm_msg(
			ERR, "executeModule", "Execution of module failed: %s",
			arg1);
		perror("");
		close(app_to_fvwm[1]);
		/* dont't care that this may be -1 */
		close(fvwm_to_app[0]);
#ifdef FORK_CREATES_CHILD
		exit(1);
#endif
	}
	else
	{
		fvwm_msg(ERR, "executeModule", "Fork failed");
		free(arg1);
		for (i = 6; i < nargs; i++)
		{
			if (args[i] != 0)
			{
				free(args[i]);
			}
		}
		free(args);
		module_remove(module);
		module_free(module);

		return NULL;
	}
	free(args);

	return module;
}

fmodule *executeModuleDesperate(F_CMD_ARGS)
{
	return do_execute_module(F_PASS_ARGS, True, False);
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

void CMD_ModuleSynchronous(F_CMD_ARGS)
{
	int sec = 0;
	char *next;
	char *token;
	char *expect = ModuleFinishedStartupResponse;
	fmodule *module;
	fd_set in_fdset;
	fd_set out_fdset;
	Window targetWindow;
	extern fd_set_size_t fd_width;
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
			expect = alloca(strlen(token) + 1);
			strcpy(expect, token);
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
			fvwm_msg(ERR, "executeModuleSync", "illegal timeout");
			return;
		}
	}

	if (!action)
	{
		/* no module name */
		return;
	}

	module = do_execute_module(F_PASS_ARGS, False, False);
	if (module == NULL)
	{
		/* executing the module failed, just return */
		return;
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
			module_add_to_fdsets(module, &in_fdset, &out_fdset);

			timeout.tv_sec = 0;
			timeout.tv_usec = 1;
			num_fd = fvwmSelect(
				fd_width, &in_fdset, &out_fdset, 0, &timeout);
		} while (num_fd < 0 && !isTerminated);

		/* Exit if we have received a "terminate" signal */
		if (isTerminated)
		{
			break;
		}

		if (num_fd > 0)
		{
			if ((module->readPipe >= 0) &&
			    FD_ISSET(module->readPipe, &in_fdset))
			{
				/* Check for module input. */
				if (read(module->readPipe, &targetWindow,
					 sizeof(Window)) > 0)
				{
					if (HandleModuleInput(
						    targetWindow, module,
						    expect, False))
					{
						/* we got the message we were
						 * waiting for */
						done = True;
					}
				}
				else
				{
					KillModule(module);
					done = True;
				}
			}

			if ((module->writePipe >= 0) &&
			    FD_ISSET(module->writePipe, &out_fdset))
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

			context = GetContext(
				NULL, exc->w.fw, &tmpevent, &targetWindow);
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
				Scr.AllBindings, STROKE_ARG(0)
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

	return;
}

/* run the command as if it cames from a button press or release */
static void ExecuteModuleCommand(Window w, fmodule *module, char *text)
{
	XEvent e;
	const exec_context_t *exc;
	exec_context_changes_t ecc;
	int flags;

	memset(&e, 0, sizeof(e));
	if (XFindContext(dpy, w, FvwmContext, (caddr_t *)&ecc.w.fw) == XCNOENT)
	{
		ecc.w.fw = NULL;
		w = None;
	}
	/* Query the pointer, the pager-drag-out feature doesn't work properly.
	 * This is OK now that the Pager uses "Move pointer"
	 * A real fix would be for the modules to pass the button press coords
	 */
	if (FQueryPointer(
		    dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX,&JunkY,
		    &e.xbutton.x_root, &e.xbutton.y_root, &e.xbutton.state) ==
	    False)
	{
		/* pointer is not on this screen */
		/* If a module does XUngrabPointer(), it can now get proper
		 * Popups */
		e.xbutton.window = Scr.Root;
		ecc.w.fw = NULL;
	}
	else
	{
		e.xbutton.window = w;
	}
	e.xbutton.subwindow = None;
	e.xbutton.button = 1;
	/* If a module does XUngrabPointer(), it can now get proper Popups */
	if (StrEquals(text, "popup"))
	{
		e.xbutton.type = ButtonPress;
		e.xbutton.state |= Button1Mask;
	}
	else
	{
		e.xbutton.type = ButtonRelease;
		e.xbutton.state &= (~(Button1Mask));
	}
	e.xbutton.x = 0;
	e.xbutton.y = 0;
	fev_fake_event(&e);
	ecc.type = EXCT_MODULE;
	ecc.w.w = w;
	flags = (w == None) ? 0 : FUNC_DONT_DEFER;
	ecc.w.wcontext = GetContext(NULL, ecc.w.fw, &e, &w);
	ecc.x.etrigger = &e;
	ecc.m.module = module;
	exc = exc_create_context(
		&ecc, ECC_TYPE | ECC_ETRIGGER | ECC_FW | ECC_W | ECC_WCONTEXT |
		ECC_MODULE);
	execute_function(NULL, exc, text, 0);
	exc_destroy_context(exc);

	return;
}

/* read input from a module and either execute it or queue it
 * returns True and does NOP if the module input matches the expect string */
Bool HandleModuleInput(Window w, fmodule *module, char *expect, Bool queue)
{
	char text[MAX_MODULE_INPUT_TEXT_LEN];
	unsigned long size;
	unsigned long cont;
	int n;

	/* Already read a (possibly NULL) window id from the pipe,
	 * Now read an fvwm bultin command line */
	n = read(module->readPipe, &size, sizeof(size));
	if (n < sizeof(size))
	{
		fvwm_msg(
			ERR, "HandleModuleInput",
			"Fail to read (Module: %p, read: %i, size: %i)",
			module, n, (int)sizeof(size));
		KillModule(module);
		return False;
	}

	if (size > sizeof(text))
	{
		fvwm_msg(ERR, "HandleModuleInput",
			 "Module(%p) command is too big (%ld), limit is %d",
			 module, size, (int)sizeof(text));
		/* The rest of the output from this module is going to be
		 * scrambled so let's kill it rather than risk interpreting
		 * garbage */
		KillModule(module);
		return False;
	}

	n = read(module->readPipe, text, size);
	if (n < size)
	{
		fvwm_msg(
			ERR, "HandleModuleInput",
			"Fail to read command (Module: %p, read: %i, size:"
			" %ld)", module, n, size);
		KillModule(module);
		return False;
	}
	text[n] = '\0';
	n = read(module->readPipe, &cont, sizeof(cont));
	if (n < sizeof(cont))
	{
		fvwm_msg(ERR, "HandleModuleInput",
			 "Module %p, Size Problems (read: %d, size: %d)",
			 module, n, (int)sizeof(cont));
		KillModule(module);
		return False;
	}
	if (cont == 0)
	{
		/* this is documented as a valid way for a module to quit
		 * so let's not complain */
		KillModule(module);
	}
	if (strlen(text)>0)
	{
		if (expect && (strncasecmp(text, expect, strlen(expect)) == 0))
		{
			/* the module sent the expected string */
			return True;
		}

		if (queue)
		{
			AddToCommandQueue(w, module, text);
		}
		else
		{
			ExecuteModuleCommand(w, module, text);
		}
	}

	return False;
}

RETSIGTYPE DeadPipe(int sig)
{
	SIGNAL_RETURN;
}

void KillModule(fmodule *module)
{
	/* remove from the list */
	module_remove(module);
	/* free it */
	module_free(module);
	if (fFvwmInStartup)
	{
		/* remove from list of command line modules */
		DBUG("KillModule", "ending command line module");
		module->flags.is_cmdline_module = 0;
	}

	return;
}

static void KillModuleByName(char *name, char *alias)
{
	fmodule *module;

	if (name == NULL)
	{
		return;
	}
	module = module_get_next(NULL);
	for (; module != NULL; module = module_get_next(module))
	{
		if (
			module->name != NULL &&
			matchWildcards(name, module->name) &&
			(!alias || (
				 module->alias &&
				 matchWildcards(alias, module->alias))))
		{
			KillModule(module);
		}
	}

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

static unsigned long *
make_vpacket(unsigned long *body, unsigned long event_type,
	     unsigned long num, va_list ap)
{
	unsigned long *bp = body;

	/* truncate long packets */
	if (num > FvwmPacketMaxSize)
	{
		num = FvwmPacketMaxSize;
	}
	*(bp++) = START_FLAG;
	*(bp++) = event_type;
	*(bp++) = num+FvwmPacketHeaderSize;
	*(bp++) = fev_get_evtime();

	for (; num > 0; --num)
	{
		*(bp++) = va_arg(ap, unsigned long);
	}

	return body;
}



/*
    RBW - 04/16/1999 - new packet builder for GSFR --
    Arguments are pairs of lengths and argument data pointers.
    RBW - 05/01/2000 -
    A length of zero means that an int is being passed which
    must be stored in the packet as an unsigned long. This is
    a special hack to accommodate the old CONFIGARGS
    technique of sending the args for the M_CONFIGURE_WINDOW
    packet.
*/
static unsigned long
make_new_vpacket(unsigned char *body, unsigned long event_type,
		 unsigned long num, va_list ap)
{
	long arglen;
	unsigned long addlen;
	unsigned long bodylen = 0;
	unsigned long *bp = (unsigned long *)body;
	unsigned long *bp1 = bp;
	unsigned long plen = 0;

	*(bp++) = START_FLAG;
	*(bp++) = event_type;
	/*  Skip length field, we don't know it yet. */
	bp++;
	*(bp++) = fev_get_evtime();

	for (; num > 0; --num)
	{
		arglen = va_arg(ap, long);
		if (arglen <= 0)
		{
			if (arglen == 0)
			{
				arglen = -sizeof(int);
			}
			addlen = sizeof(unsigned long);
		}
		else
		{
			addlen = arglen;
		}
		bodylen += addlen;
		if (bodylen >= FvwmPacketMaxSize_byte)
		{
			fvwm_msg(
				ERR, "make_new_vpacket",
				"packet too long %ld %ld", (long)bodylen,
				(long)FvwmPacketMaxSize_byte);
			break;
		}
		if (arglen > 0)
		{
			register char *tmp = (char *)bp;
			memcpy(tmp, va_arg(ap, char *), arglen);
			tmp += arglen;
			bp = (unsigned long *)tmp;
		}
		else if (arglen == 0 || arglen == -sizeof(int))
		{
			int *tmp;

			tmp = va_arg(ap, int *);
			*bp = (unsigned long) *tmp;
			bp++;
		}
		else if (arglen == -sizeof(long))
		{
			unsigned long *tmp;

			tmp = va_arg(ap, unsigned long *);
			*bp = (unsigned long) *tmp;
			bp++;
		}
		else if (arglen == -sizeof(short))
		{
			short *tmp;

			tmp = va_arg(ap, short *);
			*bp = (unsigned long) *tmp;
			bp++;
		}
		else
		{
			fvwm_msg(
				ERR, "make_new_vpacket",
				"can not handle arglen %ld, please contact"
				" fvwm-workers@fvwm.org. aborting...", arglen);
			abort();
		}
	}

	/*
	  Round up to a long word boundary. Most of the module interface
	  still thinks in terms of an array of long ints, so let's humor it.
	*/
	plen = (unsigned long) ((char *)bp - (char *)bp1);
	plen = ((plen + (sizeof(long) - 1)) / sizeof(long)) * sizeof(long);
	*(((unsigned long*)bp1)+2) = (plen / (sizeof(unsigned long)));

	return plen;
}



void SendPacket(
	fmodule *module, unsigned long event_type, unsigned long num_datum,
	...)
{
	unsigned long body[FvwmPacketMaxSize];
	va_list ap;

	va_start(ap, num_datum);
	make_vpacket(body, event_type, num_datum, ap);
	va_end(ap);
	PositiveWrite(
		module, body,
		(num_datum+FvwmPacketHeaderSize)*sizeof(body[0]));

	return;
}

void BroadcastPacket(unsigned long event_type, unsigned long num_datum, ...)
{
	unsigned long body[FvwmPacketMaxSize];
	va_list ap;
	fmodule *module;

	va_start(ap,num_datum);
	make_vpacket(body, event_type, num_datum, ap);
	va_end(ap);
	module = module_get_next(NULL);
	for (; module != NULL; module = module_get_next(module))
	{
		PositiveWrite(
			module, body,
			(num_datum+FvwmPacketHeaderSize)*sizeof(body[0]));
	}

	return;
}


/*
   RBW - 04/16/1999 - new style packet senders for GSFR --
*/
static void SendNewPacket(
	fmodule *module, unsigned long event_type, unsigned long num_datum,
	...)
{
	unsigned char body[FvwmPacketMaxSize_byte];
	va_list ap;
	unsigned long plen;

	va_start(ap,num_datum);
	plen = make_new_vpacket(body, event_type, num_datum, ap);
	va_end(ap);
	PositiveWrite(module, (void *) &body, plen);

	return;
}

static void BroadcastNewPacket(unsigned long event_type,
			       unsigned long num_datum, ...)
{
	unsigned char body[FvwmPacketMaxSize_byte];
	va_list ap;
	fmodule *module;
	unsigned long plen;

	va_start(ap,num_datum);
	plen = make_new_vpacket(body, event_type, num_datum, ap);
	va_end(ap);
	module = module_get_next(NULL);
	for (; module != NULL; module = module_get_next(module))
	{
		PositiveWrite(module, (void *) &body, plen);
	}

	return;
}

action_flags *__get_allowed_actions(const FvwmWindow *fw)
{
	static action_flags act;
	act.is_movable = is_function_allowed(F_MOVE, NULL, fw, True, False);
	act.is_deletable = is_function_allowed(F_DELETE, NULL, fw, True,
					       False);
	act.is_destroyable = is_function_allowed(F_DESTROY, NULL, fw, True,
						 False);
	act.is_closable = is_function_allowed(F_CLOSE, NULL, fw, True, False);
	act.is_maximizable = is_function_allowed(F_MAXIMIZE, NULL, fw, True,
						 False);
	act.is_resizable = is_function_allowed(F_RESIZE, NULL, fw, True,
					       False);
	act.is_iconifiable = is_function_allowed(F_ICONIFY, NULL, fw, True,
						 False);

	return &act;
}

/*
    RBW - 04/16/1999 - new version for GSFR --
	- args are now pairs:
	  - length of arg data
	  - pointer to arg data
	- number of arguments is the number of length/pointer pairs.
	- the 9th field, where flags used to be, is temporarily left
	as a dummy to preserve alignment of the other fields in the
	old packet: we should drop this before the next release.
*/
#define CONFIGARGS(_fw) 31,\
		(unsigned long)(-sizeof(Window)),	\
		&FW_W(*(_fw)),				\
		(unsigned long)(-sizeof(Window)),	\
		&FW_W_FRAME(*(_fw)),			\
		(unsigned long)(-sizeof(void *)),	\
		&(_fw),					\
		(unsigned long)(0),			\
		&(*(_fw))->g.frame.x,			\
		(unsigned long)(0),			\
		&(*(_fw))->g.frame.y,			\
		(unsigned long)(0),			\
		&(*(_fw))->g.frame.width,		\
		(unsigned long)(0),			\
		&(*(_fw))->g.frame.height,		\
		(unsigned long)(0),			\
		&(*(_fw))->Desk,			\
		(unsigned long)(0),			\
		&(*(_fw))->layer,			\
		(unsigned long)(0),			\
		&(*(_fw))->hints.base_width,		\
		(unsigned long)(0),			\
		&(*(_fw))->hints.base_height,		\
		(unsigned long)(0),			\
		&(*(_fw))->hints.width_inc,		\
		(unsigned long)(0),			\
		&(*(_fw))->hints.height_inc,		\
		(unsigned long)(0),			\
		&(*(_fw))->hints.min_width,		\
		(unsigned long)(0),			\
		&(*(_fw))->hints.min_height,		\
		(unsigned long)(0),			\
		&(*(_fw))->hints.max_width,		\
		(unsigned long)(0),			\
		&(*(_fw))->hints.max_height,		\
		(unsigned long)(-sizeof(Window)),	\
		&FW_W_ICON_TITLE(*(_fw)),		\
		(unsigned long)(-sizeof(Window)),	\
		&FW_W_ICON_PIXMAP(*(_fw)),		\
		(unsigned long)(0),			\
		&(*(_fw))->hints.win_gravity,		\
		(unsigned long)(-sizeof(Pixel)),	\
		&(*(_fw))->colors.fore,			\
		(unsigned long)(-sizeof(Pixel)),	\
		&(*(_fw))->colors.back,			\
		(unsigned long)(0),			\
		&(*(_fw))->ewmh_hint_layer,		\
		(unsigned long)(sizeof(unsigned long)),	\
		&(*(_fw))->ewmh_hint_desktop,		\
		(unsigned long)(0),			\
		&(*(_fw))->ewmh_window_type,		\
		(unsigned long)(sizeof(short)),		\
		&(*(_fw))->title_thickness,		\
		(unsigned long)(sizeof(short)),		\
		&(*(_fw))->boundary_width,		\
		(unsigned long)(sizeof(short)),		\
		&dummy,					\
		(unsigned long)(sizeof(short)),		\
		&dummy,					\
		(unsigned long)(sizeof((*(_fw))->flags)),	\
		&(*(_fw))->flags,                       \
		(unsigned long)(sizeof(action_flags)),  \
		__get_allowed_actions((*(_fw)))

void SendConfig(fmodule *module, unsigned long event_type, const FvwmWindow *t)
{
	const FvwmWindow **t1 = &t;

	/*  RBW-  SendPacket(module, event_type, CONFIGARGS(t)); */
	SendNewPacket(module, event_type, CONFIGARGS(t1));

	return;
}

void BroadcastConfig(unsigned long event_type, const FvwmWindow *t)
{
	const FvwmWindow **t1 = &t;

	/*  RBW-  BroadcastPacket(event_type, CONFIGARGS(t)); */
	BroadcastNewPacket(event_type, CONFIGARGS(t1));

	return;
}

static unsigned long *make_named_packet(
	int *len, unsigned long event_type, const char *name, int num, ...)
{
	unsigned long *body;
	va_list ap;

	/* Packet is the header plus the items plus enough items to hold the
	 * name string. */
	*len = FvwmPacketHeaderSize + num +
		(strlen(name) / sizeof(unsigned long)) + 1;
	/* truncate long packets */
	if (*len > FvwmPacketMaxSize)
	{
		*len = FvwmPacketMaxSize;
	}

	body = (unsigned long *)safemalloc(*len * sizeof(unsigned long));
	/* Zero out end of memory to avoid uninit memory access. */
	body[*len-1] = 0;

	va_start(ap, num);
	make_vpacket(body, event_type, num, ap);
	va_end(ap);

	strncpy((char *)&body[FvwmPacketHeaderSize+num], name,
		(*len - FvwmPacketHeaderSize - num)*sizeof(unsigned long) - 1);
	body[2] = *len;

	return (body);
}

void SendName(
	fmodule *module, unsigned long event_type,
	 unsigned long data1,unsigned long data2, unsigned long data3,
	 const char *name)
{
	unsigned long *body;
	int l;

	if (name == NULL)
	{
		return;
	}
	body = make_named_packet(&l, event_type, name, 3, data1, data2, data3);
	PositiveWrite(module, body, l*sizeof(unsigned long));
	free(body);

	return;
}

void BroadcastName(
	unsigned long event_type,
	      unsigned long data1, unsigned long data2, unsigned long data3,
	      const char *name)
{
	unsigned long *body;
	int l;
	fmodule *module;

	if (name == NULL)
	{
		return;
	}
	body = make_named_packet(&l, event_type, name, 3, data1, data2, data3);
	module = module_get_next(NULL);
	for (; module != NULL; module = module_get_next(module))
	{
		PositiveWrite(module, body, l*sizeof(unsigned long));
	}
	free(body);

	return;
}

void BroadcastWindowIconNames(FvwmWindow *fw, Bool window, Bool icon)
{
	if (window)
	{
		BroadcastName(
			M_WINDOW_NAME, FW_W(fw), FW_W_FRAME(fw),
			(unsigned long)fw, fw->name.name);
		BroadcastName(
			M_VISIBLE_NAME, FW_W(fw), FW_W_FRAME(fw),
			(unsigned long)fw, fw->visible_name);
	}
	if (icon)
	{
		BroadcastName(
			M_ICON_NAME, FW_W(fw), FW_W_FRAME(fw),
			(unsigned long)fw, fw->icon_name.name);
		BroadcastName(
			MX_VISIBLE_ICON_NAME, FW_W(fw), FW_W_FRAME(fw),
			(unsigned long)fw, fw->visible_icon_name);
	}

	return;
}

void SendFvwmPicture(
	fmodule *module, unsigned long event_type, unsigned long data1,
	unsigned long data2, unsigned long data3, FvwmPicture *picture,
	char *name)
{
	unsigned long *body;
	unsigned long
		data4 = 0, data5 = 0, data6 = 0,
		data7 = 0, data8 = 0, data9 = 0;
	int l;

	if (!FMiniIconsSupported)
	{
		return;
	}
	if ((name == NULL) || (event_type != M_MINI_ICON))
	{
		return;
	}

	if (picture != NULL)
	{
		data4 = picture->width;
		data5 = picture->height;
		data6 = picture->depth;
		data7 = picture->picture;
		data8 = picture->mask;
		data9 = picture->alpha;
	}
	body = make_named_packet(
		&l, event_type, name, 9, data1, data2, data3, data4, data5,
		data6, data7, data8, data9);
	PositiveWrite(module, body, l*sizeof(unsigned long));
	free(body);

	return;
}

void BroadcastFvwmPicture(
	unsigned long event_type, unsigned long data1, unsigned long data2,
	unsigned long data3, FvwmPicture *picture, char *name)
{
	unsigned long *body;
	unsigned long data4, data5, data6, data7, data8, data9;
	int l;
	fmodule *module;

	if (!FMiniIconsSupported)
	{
		return;
	}
	if (picture != NULL)
	{
		data4 = picture->width;
		data5 = picture->height;
		data6 = picture->depth;
		data7 = picture->picture;
		data8 = picture->mask;
		data9 = picture->alpha;
	}
	else
	{
		data4 = 0;
		data5 = 0;
		data6 = 0;
		data7 = 0;
		data8 = 0;
		data9 = 0;
	}
	body = make_named_packet(
		&l, event_type, name, 9, data1, data2, data3, data4, data5,
		data6, data7, data8, data9);
	module = module_get_next(NULL);
	for (; module != NULL; module = module_get_next(module))
	{
		PositiveWrite(module, body, l*sizeof(unsigned long));
	}
	free(body);

	return;
}

/*
 * Reads a colorset command from a module and broadcasts it back out
 */
void BroadcastColorset(int n)
{
	fmodule *module;
	char *buf;

	buf = DumpColorset(n, &Colorset[n]);
	module = module_get_next(NULL);
	for (; module != NULL; module = module_get_next(module))
	{
		SendName(module, M_CONFIG_INFO, 0, 0, 0, buf);
	}

	return;
}

/*
 * Broadcasts a string to all modules as M_CONFIG_INFO.
 */
void BroadcastPropertyChange(
	unsigned long argument, unsigned long data1, unsigned long data2,
	char *string)
{
	fmodule *module;

	module = module_get_next(NULL);
	for (; module != NULL; module = module_get_next(module))
	{
		SendName(module, MX_PROPERTY_CHANGE, argument, data1, data2,
				string);
		}

	return;
}

/*
 * Broadcasts a string to all modules as M_CONFIG_INFO.
 */
void BroadcastConfigInfoString(char *string)
{
	fmodule *module;

	module = module_get_next(NULL);
	for (; module != NULL; module = module_get_next(module))
	{
		SendName(module, M_CONFIG_INFO, 0, 0, 0, string);
	}

	return;
}


/*
 * Broadcasts the state of Xinerama support to all modules as M_CONFIG_INFO.
 */
void broadcast_xinerama_state(void)
{
	BroadcastConfigInfoString((char *)FScreenGetConfiguration());

	return;
}


/*
 * Broadcasts the ignored modifiers to all modules as M_CONFIG_INFO.
 */
void broadcast_ignore_modifiers(void)
{
	char msg[32];

	sprintf(msg, "IgnoreModifiers %d", GetUnusedModifiers());
	BroadcastConfigInfoString(msg);

	return;
}


/*
** send an arbitrary string to all instances of a module
*/
void CMD_SendToModule(F_CMD_ARGS)
{
	char *name,*str;
	unsigned long data0, data1, data2;
	fmodule *module;
	FvwmWindow * const fw = exc->w.fw;

	/* FIXME: Without this, popup menus can't be implemented properly in
	 *  modules.  Olivier: Why ? */
	/* UngrabEm(); */
	if (!action)
	{
		return;
	}
	str = GetNextToken(action, &name);
	if (!name)
	{
		return;
	}

	if (fw)
	{
		/* Modules may need to know which window this applies to */
		data0 = FW_W(fw);
		data1 = FW_W_FRAME(fw);
		data2 = (unsigned long)fw;
	}
	else
	{
		data0 = 0;
		data1 = 0;
		data2 = 0;
	}

	module = module_get_next(NULL);
	for (; module != NULL; module = module_get_next(module))
	{
		if (
			(module->name != NULL &&
			 matchWildcards(name,module->name)) ||
			(module->alias &&
			 matchWildcards(name, module->alias)))
		{
			SendName(module,M_STRING,data0,data1,data2,str);
			FlushMessageQueue(module);
		}
	}

	free(name);

	return;
}

/*
** send an arbitrary string back to the calling module
*/
void CMD_Send_Reply(F_CMD_ARGS)
{
	unsigned long data0, data1, data2;
	fmodule *module = exc->m.module;
	FvwmWindow * const fw = exc->w.fw;

	if (module == NULL)
	{
		return;
	}

	if (!action)
	{
		return;
	}

	if (fw)
	{
		/* Modules may need to know which window this applies to */
		data0 = FW_W(fw);
		data1 = FW_W_FRAME(fw);
		data2 = (unsigned long)fw;
	}
	else
	{
		data0 = 0;
		data1 = 0;
		data2 = 0;
	}
	SendName(module, MX_REPLY, data0, data1, data2, action);
	FlushMessageQueue(module);

	return;
}

/* This used to be marked "fvwm_inline".  I removed this
   when I added the lockonsend logic.  The routine seems too big to
   want to inline.  dje 9/4/98 */
extern int myxgrabcount;                /* defined in libs/Grab.c */
extern char *ModuleUnlock;              /* defined in libs/Module.c */
void PositiveWrite(fmodule *module, unsigned long *ptr, int size)
{
	extern int moduleTimeout;
	msg_masks_t mask;

	if (ptr == NULL)
	{
		return;
	}
	if (module->writePipe == -1)
	{
		return;
	}
	if (!IS_MESSAGE_IN_MASK(&(module->PipeMask), ptr[1]))
	{
		return;
	}

	/* a dirty hack to prevent FvwmAnimate triggering during Recapture */
	/* would be better to send RecaptureStart and RecaptureEnd messages. */
	/* If module is lock on send for iconify message and it's an
	 * iconify event and server grabbed, then return */
	mask.m1 = (module->NoGrabMask.m1 & module->SyncMask.m1);
	mask.m2 = (module->NoGrabMask.m2 & module->SyncMask.m2);
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
			fvwm_msg(ERR, "PositiveWrite", "malloc failed\n");
			exit(1);
		}
		c->size = size;
		c->done = 0;
		c->data = (unsigned long *)(c + 1);
		memcpy((void*)c->data, (const void*)ptr, size);
		fqueue_add_at_end(&(module->pipeQueue), c);
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
	/*if (IS_MESSAGE_IN_MASK(&(module->SyncMask), ptr[1]))*/
	if (IS_MESSAGE_IN_MASK(&(module->SyncMask), ptr[1]) && !myxgrabcount)
	{
		Window targetWindow;
		fd_set readSet;
		int channel = module->readPipe;
		struct timeval timeout;

		FlushMessageQueue(module);
		if (module->readPipe < 0)
		{
			/* Module has died, break out */
			return;
		}

		do
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

			if (rc <= 0 || read(channel, &targetWindow,
					    sizeof(targetWindow))
			    != sizeof(targetWindow))
			{
				char *name;

				name = get_pipe_name(module);
				/* Doh! Something has gone wrong - get rid of
				 * the offender! */
				fvwm_msg(ERR, "PositiveWrite",
					 "Failed to read descriptor from"
					 " '%s':\n"
					 "- data available=%c\n"
					 "- terminate signal=%c\n",
					 name,
					 (FD_ISSET(channel, &readSet) ?
					  'Y' : 'N'),
					 isTerminated ? 'Y' : 'N');
				KillModule(module);
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
		while (
			!HandleModuleInput(
				targetWindow, module, ModuleUnlockResponse,
				False));
	}

	return;
}


static void DeleteMessageQueueBuff(fmodule *module)
{
	mqueue_object_type *obj;

	if (fqueue_get_first(&(module->pipeQueue), (void **)&obj) == 1)
	{
		/* remove from queue */
		fqueue_remove_or_operate_from_front(
			&(module->pipeQueue), NULL, NULL, NULL, NULL);
		/* we don't need to free the obj->data here because it's in the
		 * same malloced block as the obj itself. */
		free(obj);
	}

	return;
}

void FlushMessageQueue(fmodule *module)
{
	extern int moduleTimeout;
	mqueue_object_type *obj;
	char *dptr;
	int a;

	if (module == NULL)
	{
		return;
	}

	while (fqueue_get_first(&(module->pipeQueue), (void **)&obj) == 1)
	{
		dptr = (char *)obj->data;
		while (obj->done < obj->size)
		{
			a = write(module->writePipe, &dptr[obj->done],
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
				int channel = module->writePipe;
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
					fvwm_msg(
						ERR, "FlushMessageQueue",
						"Failed to write descriptor to"
						" '%s':\n"
						"- select rc=%d\n"
						"- terminate signal=%c\n",
						name, rc, isTerminated ?
						'Y' : 'N');
					KillModule(module);
					return;
				}

				/* pipe accepts further input; continue */
				continue;
			}
			else if (errno != EINTR)
			{
				KillModule(module);
				return;
			}
		}
		DeleteMessageQueueBuff(module);
	}

	return;
}

void FlushAllMessageQueues(void)
{
	fmodule *module;

	module = module_get_next(NULL);
	for (; module != NULL; module = module_get_next(module))
		{
		FlushMessageQueue(module->next);
	}

	return;
}

/* A queue of commands from the modules */
typedef struct
{
	Window window;
	fmodule *module;
	char *command;
} cqueue_object_type;

static fqueue cqueue = FQUEUE_INIT;

/*
 *
 *  Procedure:
 *      AddToCommandQueue - add a module command to the command queue
 *
 */

static void AddToCommandQueue(Window window, fmodule *module, char *command)
{
	cqueue_object_type *newqo;

	if (!command)
	{
		return;
	}

	newqo = (cqueue_object_type *)safemalloc(sizeof(cqueue_object_type));
	newqo->window = window;
	newqo->module = module;
	newqo->command = safestrdup(command);
	DBUG("AddToCommandQueue", command);
	fqueue_add_at_end(&cqueue, newqo);

	return;
}

/*
 *
 *  Procedure:
 *      ExecuteCommandQueue - runs command from the module command queue
 *      This may be called recursively if a module command runs a function
 *      that does a Wait, so it must be re-entrant
 *
 */
void ExecuteCommandQueue(void)
{
	cqueue_object_type *obj;

	while (fqueue_get_first(&cqueue, (void **)&obj) == 1)
	{
		/* remove from queue */
		fqueue_remove_or_operate_from_front(
			&cqueue, NULL, NULL, NULL, NULL);
		/* execute and destroy */
		if (obj->command)
		{
			DBUG("EmptyCommandQueue", obj->command);
			ExecuteModuleCommand(
				obj->window, obj->module, obj->command);
			free(obj->command);
		}
		free(obj);
	}

	return;
}

void CMD_Send_WindowList(F_CMD_ARGS)
{
	FvwmWindow *t;
	fmodule *mod = exc->m.module;

	if (mod == NULL)
	{
		return;
	}
	SendPacket(mod, M_NEW_DESK, 1, (long)Scr.CurrentDesk);
	SendPacket(
		mod, M_NEW_PAGE, 7, (long)Scr.Vx, (long)Scr.Vy,
		(long)Scr.CurrentDesk, (long)Scr.MyDisplayWidth,
		(long)Scr.MyDisplayHeight,
		(long)((Scr.VxMax / Scr.MyDisplayWidth) + 1),
		(long)((Scr.VyMax / Scr.MyDisplayHeight) + 1));

	if (Scr.Hilite != NULL)
	{
		SendPacket(
			mod, M_FOCUS_CHANGE, 5, (long)FW_W(Scr.Hilite),
			(long)FW_W_FRAME(Scr.Hilite), (unsigned long)True,
			(long)Scr.Hilite->hicolors.fore,
			(long)Scr.Hilite->hicolors.back);
	}
	else
	{
		SendPacket(
			mod, M_FOCUS_CHANGE, 5, 0, 0, (unsigned long)True,
			(long)GetColor(DEFAULT_FORE_COLOR),
			(long)GetColor(DEFAULT_BACK_COLOR));
	}
	if (Scr.DefaultIcon != NULL)
	{
		SendName(mod, M_DEFAULTICON, 0, 0, 0, Scr.DefaultIcon);
	}

	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		SendConfig(mod,M_CONFIGURE_WINDOW,t);
		SendName(
			mod, M_WINDOW_NAME, FW_W(t), FW_W_FRAME(t),
			(unsigned long)t, t->name.name);
		SendName(
			mod, M_ICON_NAME, FW_W(t), FW_W_FRAME(t),
			(unsigned long)t, t->icon_name.name);
		SendName(
			mod, M_VISIBLE_NAME, FW_W(t), FW_W_FRAME(t),
			(unsigned long)t, t->visible_name);
		SendName(
			mod, MX_VISIBLE_ICON_NAME, FW_W(t), FW_W_FRAME(t),
			(unsigned long)t,t->visible_icon_name);
		if (t->icon_bitmap_file != NULL
		    && t->icon_bitmap_file != Scr.DefaultIcon)
		{
			SendName(
				mod, M_ICON_FILE, FW_W(t), FW_W_FRAME(t),
				(unsigned long)t, t->icon_bitmap_file);
		}

		SendName(
			mod, M_RES_CLASS, FW_W(t), FW_W_FRAME(t),
			(unsigned long)t, t->class.res_class);
		SendName(
			mod, M_RES_NAME, FW_W(t), FW_W_FRAME(t),
			(unsigned long)t, t->class.res_name);

		if (IS_ICONIFIED(t) && !IS_ICON_UNMAPPED(t))
		{
			rectangle r;
			Bool rc;

			rc = get_visible_icon_geometry(t, &r);
			if (rc == True)
			{
				SendPacket(
					mod, M_ICONIFY, 7, (long)FW_W(t),
					(long)FW_W_FRAME(t), (unsigned long)t,
					(long)r.x, (long)r.y,
					(long)r.width, (long)r.height);
			}
		}
		if ((IS_ICONIFIED(t))&&(IS_ICON_UNMAPPED(t)))
		{
			SendPacket(
				mod, M_ICONIFY, 7, (long)FW_W(t),
				(long)FW_W_FRAME(t), (unsigned long)t,
				(long)0, (long)0, (long)0, (long)0);
		}
		if (FMiniIconsSupported && t->mini_icon != NULL)
		{
			SendFvwmPicture(
				mod, M_MINI_ICON, FW_W(t), FW_W_FRAME(t),
				(unsigned long)t, t->mini_icon,
				t->mini_pixmap_file);
		}
	}

	if (Scr.Hilite == NULL)
	{
		BroadcastPacket(
			M_FOCUS_CHANGE, 5, (long)0, (long)0,
			(unsigned long)True,
			(long)GetColor(DEFAULT_FORE_COLOR),
			(long)GetColor(DEFAULT_BACK_COLOR));
	}
	else
	{
		BroadcastPacket(
			M_FOCUS_CHANGE, 5, (long)FW_W(Scr.Hilite),
			(long)FW_W(Scr.Hilite), (unsigned long)True,
			(long)Scr.Hilite->hicolors.fore,
			(long)Scr.Hilite->hicolors.back);
	}

	SendPacket(mod, M_END_WINDOWLIST, 0);

	return;
}

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
	set_message_mask(&(exc->m.module->PipeMask), (unsigned long)val);

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
	set_message_mask(&(exc->m.module->SyncMask), (unsigned long)val);

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
	set_message_mask(&(exc->m.module->NoGrabMask), (unsigned long)val);

	return;
}

/*
 * returns a pointer inside a string (just after the alias) if ok or NULL
 */
char *skipModuleAliasToken(const char *string)
{
#define is_valid_first_alias_char(ch) (isalpha(ch) || (ch) == '/')
#define is_valid_alias_char(ch) (is_valid_first_alias_char(ch) \
	|| isalnum(ch) || (ch) == '-' || (ch) == '.' || (ch) == '/')

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

int countModules(void)
{
	return num_modules;
}
