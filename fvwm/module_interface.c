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

/*
 *
 * code for talking with fvwm modules.
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include "libs/ftime.h"
#include "libs/fvwmlib.h"
#include "libs/ColorUtils.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/wild.h"
#include "fvwm.h"
#include "externs.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "module_list.h"
#include "events.h"
#include "geometry.h"
#include "libs/fvwmsignal.h"
#include "decorations.h"
#include "commands.h"

/* A queue of commands from the modules */
static fqueue cqueue = FQUEUE_INIT;

static const unsigned long dummy = 0;

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
	  still thinks in terms of an array of longss, so let's humor it.
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
	fmodule_list_itr moditr;
	fmodule *module;

	va_start(ap,num_datum);
	make_vpacket(body, event_type, num_datum, ap);
	va_end(ap);
	module_list_itr_init(&moditr);
	while ( (module = module_list_itr_next(&moditr)) != NULL)
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
	fmodule_list_itr moditr;
	fmodule *module;
	unsigned long plen;

	va_start(ap,num_datum);
	plen = make_new_vpacket(body, event_type, num_datum, ap);
	va_end(ap);
	module_list_itr_init(&moditr);
	while ( (module = module_list_itr_next(&moditr)) != NULL)
	{
		PositiveWrite(module, (void *) &body, plen);
	}

	return;
}

action_flags *__get_allowed_actions(const FvwmWindow *fw)
{
	static action_flags act;
	act.is_movable = is_function_allowed(
		F_MOVE, NULL, fw, RQORIG_PROGRAM_US, False);
	act.is_deletable = is_function_allowed(
		F_DELETE, NULL, fw, RQORIG_PROGRAM_US, False);
	act.is_destroyable = is_function_allowed(
		F_DESTROY, NULL, fw, RQORIG_PROGRAM_US, False);
	act.is_closable = is_function_allowed(
		F_CLOSE, NULL, fw, RQORIG_PROGRAM_US, False);
	act.is_maximizable = is_function_allowed(
		F_MAXIMIZE, NULL, fw, RQORIG_PROGRAM_US, False);
	act.is_resizable = is_function_allowed(
		F_RESIZE, NULL, fw, RQORIG_PROGRAM_US, False);
	act.is_iconifiable = is_function_allowed(
		F_ICONIFY, NULL, fw, RQORIG_PROGRAM_US, False);

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
#define CONFIGARGS(_fw) 34,				\
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
		&(*(_fw))->m->si->rr_output,		\
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
		&(*(_fw))->orig_hints.width_inc,	\
		(unsigned long)(0),			\
		&(*(_fw))->orig_hints.height_inc,	\
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
		&dummy,						\
		(unsigned long)(sizeof((*(_fw))->flags)),	\
		&(*(_fw))->flags,				\
		(unsigned long)(sizeof(action_flags)),		\
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

	body = fxmalloc(*len * sizeof(unsigned long));
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
	fmodule_list_itr moditr;
	fmodule *module;

	if (name == NULL)
	{
		return;
	}
	body = make_named_packet(&l, event_type, name, 3, data1, data2, data3);
	module_list_itr_init(&moditr);
	while ( (module = module_list_itr_next(&moditr)) != NULL)
	{
		PositiveWrite(module, body, l*sizeof(unsigned long));
	}
	free(body);

	return;
}

void BroadcastMonitorList(fmodule *this)
{
	char		*name;
	struct monitor	*m, *mcur;
	fmodule_list_itr moditr;
	fmodule *module;

	module_list_itr_init(&moditr);

	mcur = monitor_get_current();
	TAILQ_FOREACH(m, &monitor_q, entry) {
		if (m->si->is_disabled)
			continue;
		asprintf(&name, "Monitor %s %d %d %d %d %d %d %d %d",
			m->si->name,
			(int)m->si->rr_output,
			m == mcur,
			m->virtual_scr.MyDisplayWidth,
			m->virtual_scr.MyDisplayHeight,
			m->virtual_scr.Vx,
			m->virtual_scr.Vy,
			m->virtual_scr.VxMax,
			m->virtual_scr.VyMax
		);

		if (this != NULL)
			SendName(this, M_CONFIG_INFO, 0, 0, 0, name);
		else {
			while ((module = module_list_itr_next(&moditr)) != NULL)
				SendName(module, M_CONFIG_INFO, 0, 0, 0, name);
		}
		fprintf(stderr, "Sent: <<%s>>\n", name);
		free(name);
	}
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
	fmodule_list_itr moditr;
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
	module_list_itr_init(&moditr);
	while ( (module = module_list_itr_next(&moditr)) != NULL)
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
	fmodule_list_itr moditr;
	fmodule *module;
	char *buf;

	buf = DumpColorset(n, &Colorset[n]);
	module_list_itr_init(&moditr);
	while ( (module = module_list_itr_next(&moditr)) != NULL)
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
	fmodule_list_itr moditr;
	fmodule *module;

	module_list_itr_init(&moditr);
	while ( (module = module_list_itr_next(&moditr)) != NULL)
	{
		SendName(module, MX_PROPERTY_CHANGE, argument,
			 data1, data2, string);
	}

	return;
}

/*
 * Broadcasts a string to all modules as M_CONFIG_INFO.
 */
void BroadcastConfigInfoString(char *string)
{
	fmodule_list_itr moditr;
	fmodule *module;

	module_list_itr_init(&moditr);
	while ( (module = module_list_itr_next(&moditr)) != NULL)
	{
		SendName(module, M_CONFIG_INFO, 0, 0, 0, string);
	}

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

/* run the input command as if it cames from a button press or release */
void module_input_execute(struct fmodule_input *input)
{
	XEvent e;
	const exec_context_t *exc;
	exec_context_changes_t ecc;
	int flags;

	memset(&e, 0, sizeof(e));
	if (XFindContext(dpy, input->window, FvwmContext,
				 (caddr_t *)&ecc.w.fw) == XCNOENT)
	{
		ecc.w.fw = NULL;
		input->window = None;
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
		e.xbutton.window = input->window;
	}
	e.xbutton.subwindow = None;
	e.xbutton.button = 1;
	/* If a module does XUngrabPointer(), it can now get proper Popups */
	if (StrEquals(input->command, "popup"))
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
	ecc.w.w = input->window;
	flags = (input->window == None) ? 0 : FUNC_DONT_DEFER;
	ecc.w.wcontext = GetContext(NULL, ecc.w.fw, &e, &(input->window));
	ecc.x.etrigger = &e;
	ecc.m.module = input->module;
	exc = exc_create_context(
		&ecc, ECC_TYPE | ECC_ETRIGGER | ECC_FW | ECC_W | ECC_WCONTEXT |
		ECC_MODULE);
	execute_function(NULL, exc, input->command, flags);
	exc_destroy_context(exc);
	module_input_discard(input);

	return;
}

/* enqueue a module command on the command queue to be executed later  */
void module_input_enqueue(struct fmodule_input *input)
{
	if (input == NULL)
	{
		return;
	}

	DBUG("module_input_enqueue", input->command);
	fqueue_add_at_end(&cqueue, (void*)input);
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
	fmodule_input *input;

	while (fqueue_get_first(&cqueue, (void **)&input) == 1)
	{
		/* remove from queue */
		fqueue_remove_or_operate_from_front(
			&cqueue, NULL, NULL, NULL, NULL);
		/* execute and destroy */
		if (input->command)
		{
			DBUG("ExecuteCommandQueue", input->command);
			module_input_execute(input);
		}
		else
		{
			module_input_discard(input);
		}
	}

	return;
}

/*
** send an arbitrary string to all instances of a module
*/
void CMD_SendToModule(F_CMD_ARGS)
{
	char *name,*str;
	unsigned long data0, data1, data2;
	fmodule_list_itr moditr;
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

	module_list_itr_init(&moditr);
	while ( (module = module_list_itr_next(&moditr)) != NULL)
	{
		if (
			(MOD_NAME(module) != NULL &&
			 matchWildcards(name,MOD_NAME(module))) ||
			(MOD_ALIAS(module) &&
			 matchWildcards(name, MOD_ALIAS(module))))
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

void CMD_Send_WindowList(F_CMD_ARGS)
{
	FvwmWindow *t;
	fmodule *mod = exc->m.module;
	struct monitor	*m;

	if (mod == NULL)
	{
		return;
	}

	/* TA: 2020-01-09:  We send this for *ALL* configured monitors.
	 *
	 * Modules can filter out those monitor packets they're not interested
	 * in.
	 */
	TAILQ_FOREACH(m, &monitor_q, entry) {
		SendPacket(mod, M_NEW_DESK, 2, (long)m->virtual_scr.CurrentDesk,
			(long)m->si->rr_output);
		SendPacket(
			mod, M_NEW_PAGE, 8, (long)m->virtual_scr.Vx, (long)m->virtual_scr.Vy,
			(long)m->virtual_scr.CurrentDesk, (long)m->virtual_scr.MyDisplayWidth,
			(long)m->virtual_scr.MyDisplayHeight,
			(long)((m->virtual_scr.VxMax / m->virtual_scr.MyDisplayWidth) + 1),
			(long)((m->virtual_scr.VyMax / m->virtual_scr.MyDisplayHeight) + 1),
			(long)m->si->rr_output);

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
			if ((!(monitor_mode == MONITOR_TRACKING_G)) && t->m != m)
				continue;

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

		/* If we're tracking just the global monitor, then stop here
		 * -- essentialy, we only need to send this informatio once.
		 */
		if (monitor_mode == MONITOR_TRACKING_G)
			break;
	}

	SendPacket(mod, M_END_WINDOWLIST, 0);
}
