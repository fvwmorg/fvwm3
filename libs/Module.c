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
** Module.c: code for modules to communicate with fvwm
*/
#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include "libs/defaults.h"
#include "Module.h"
#include "Parse.h"

/*
 * Loop until count bytes are read, unless an error or end-of-file
 * condition occurs.
 */
inline
static int positive_read(int fd, char *buf, int count)
{
	while (count > 0)
	{
		int n_read = read(fd, buf, count);
		if (n_read <= 0)
		{
			return -1;
		}
		buf += n_read;
		count -= n_read;
	}
	return 0;
}


/*
 * Reads a single packet of info from fvwm.
 * The packet is stored in static memory that is reused during
 * the next call.
 */

FvwmPacket *ReadFvwmPacket(int fd)
{
	static unsigned long buffer[FvwmPacketMaxSize];
	FvwmPacket *packet = (FvwmPacket *)buffer;
	unsigned long length;

	/* The `start flag' value supposedly exists to synchronize the
	 * fvwm -> module communication.  However, the communication goes
	 * through a pipe.  I don't see how any data could ever get lost,
	 * so how would fvwm & the module become unsynchronized?
	 */
	do
	{
		if (positive_read(fd, (char *)buffer, sizeof(unsigned long))
			< 0)
		{
			return NULL;
		}
	} while (packet->start_pattern != START_FLAG);

	/* Now read the rest of the header */
	if (positive_read(fd, (char *)(&buffer[1]), 3 * sizeof(unsigned long))
		< 0)
	{
		return NULL;
	}
	length = FvwmPacketBodySize_byte(*packet);
	if (length > FvwmPacketMaxSize_byte - FvwmPacketHeaderSize_byte)
	{
		/* packet too long */
		return NULL;
	}
	/* Finally, read the body, and we're done */
	if (positive_read(fd, (char *)(&buffer[4]), length) < 0)
	{
		return NULL;
	}

	return packet;
}


/*
 *
 * SendFinishedStartupNotification - informs fvwm that the module has
 * finished its startup procedures and is fully operational now.
 *
 */
void SendFinishedStartupNotification(int *fd)
{
	SendText(fd, ModuleFinishedStartupResponse, 0);
}

/*
 *
 * SendUnlockNotification - informs fvwm that the module has
 * finished it's procedures and fvwm may proceed.
 *
 */
void SendUnlockNotification(int *fd)
{
	SendText(fd, ModuleUnlockResponse, 0);
}

/*
 *
 * SendQuitNotification - informs fvwm that the module has
 * finished and may be killed.
 *
 */
static unsigned long ModuleContinue = 1;
void SendQuitNotification(int *fd)
{
	ModuleContinue = 0;
	SendText(fd, ModuleUnlockResponse, 0); /* unlock just in case */
}

/*
 *
 * SendText - Sends arbitrary text/command back to fvwm
 *
 */
void SendText(int *fd, const char *message, unsigned long window)
{
	char *p, *buf;
	unsigned int len;

	if (!message)
	{
		return;
	}

	/* Get enough memory to store the entire message. */
	len = strlen(message);
	p = buf = alloca(sizeof(long) * (3 + 1 + (len / sizeof(long))));

	/* Put the message in the buffer, and... */
	*((unsigned long *)p) = window;
	p += sizeof(unsigned long);

	*((unsigned long *)p) = len;
	p += sizeof(unsigned long);

	strcpy(p, message);
	p += len;

	memcpy(p, &ModuleContinue, sizeof(unsigned long));
	p += sizeof(unsigned long);

	/* Send it! */
	{
		int n;

		n = write(fd[0], buf, p - buf);
		(void)n;
	}
}

/*
 *
 * SendFvwmPipe - Sends message to fvwm:  The message is a comma-delimited
 * string separated into its component sections and sent one by one to fvwm.
 * It is discouraged to use this function with a "synchronous" module.
 * (Form FvwmIconMan)
 *
 */
void SendFvwmPipe(int *fd, const char *message, unsigned long window)
{
	const char *hold = message;
	const char *temp;

	while ((temp = strchr(hold, ',')) != NULL)
	{
		char *temp_msg = (char*)alloca(temp - hold + 1);

		strncpy(temp_msg, hold, (temp - hold));
		temp_msg[(temp - hold)] = '\0';
		hold = temp + 1;

		SendText(fd, temp_msg, window);
	}

	/*
	 * Send the last part of the string :
	 * we don't need to copy this into separate
	 * storage because we don't need to modify it ...
	 *
	 * NOTE: this makes this second call to SendText()
	 *       distinct from the first call. Two calls is
	 *       cleaner than hacking the loop to make only
	 *       one call.
	 */
	SendText(fd, hold, window);
}

void SetMessageMask(int *fd, unsigned long mask)
{
	char set_mask_mesg[50];

	sprintf(set_mask_mesg, "SET_MASK %lu", mask);
	SendText(fd, set_mask_mesg, 0);
}

void SetSyncMask(int *fd, unsigned long mask)
{
	char set_syncmask_mesg[50];

	sprintf(set_syncmask_mesg, "SET_SYNC_MASK %lu", mask);
	SendText(fd, set_syncmask_mesg, 0);
}

void SetNoGrabMask(int *fd, unsigned long mask)
{
	char set_nograbmask_mesg[50];

	sprintf(set_nograbmask_mesg, "SET_NOGRAB_MASK %lu", mask);
	SendText(fd, set_nograbmask_mesg, 0);
}

/*
 * Optional routine that sets the matching criteria for config lines
 * that should be sent to a module by way of the GetConfigLine function.
 *
 * If this routine is not called, all module config lines are sent.
 */
static int first_pass = 1;

void InitGetConfigLine(int *fd, char *match)
{
	char *buffer = (char *)alloca(strlen(match) + 32);
	first_pass = 0;              /* make sure get wont do this */
	sprintf(buffer, "Send_ConfigInfo %s", match);
	SendText(fd, buffer, 0);
}


/*
 * Gets a module configuration line from fvwm. Returns NULL if there are
 * no more lines to be had. "line" is a pointer to a char *.
 *
 * Changed 10/19/98 by Dan Espen:
 *
 * - The "isspace"  call was referring to  memory  beyond the end of  the
 * input area.  This could have led to the creation of a core file. Added
 * "body_size" to keep it in bounds.
 */
void GetConfigLine(int *fd, char **tline)
{
	FvwmPacket *packet;
	int body_count;

	if (first_pass)
	{
		SendText(fd, "Send_ConfigInfo", 0);
		first_pass = 0;
	}

	do
	{
		packet = ReadFvwmPacket(fd[1]);
		if (packet == NULL || packet->type == M_END_CONFIG_INFO)
		{
			*tline = NULL;
			return;
		}
	} while (packet->type != M_CONFIG_INFO);

	/* For whatever reason CONFIG_INFO packets start with three
	 * (unsigned long) zeros.  Skip the zeros and any whitespace that
	 * follows */
	*tline = (char *)&(packet->body[3]);
	body_count = FvwmPacketBodySize(*packet) * sizeof(unsigned long);

	while (body_count > 0 && isspace((unsigned char)**tline))
	{
		(*tline)++;
		--body_count;
	}
}


ModuleArgs *ParseModuleArgs(int argc, char *argv[], int use_arg6_as_alias)
{
	static ModuleArgs ma;

	/* Need at least six arguments:
	   [0] name of executable
	   [1] file descriptor of module->fvwm pipe (write end)
	   [2] file descriptor of fvwm->module pipe (read end)
	   [3] pathname of last config file read (ignored, use Send_ConfigInfo)
	   [4] application window context
	   [5] window decoration context

	   Optionally (left column used if use_arg6_as_alias is true):
	   [6] alias       or  user argument 0
	   [7] user arg 0  or  user arg 1
	   ...
	*/
	if (argc < 6)
	{
		return NULL;
	}

	/* Module name is (last component of) argv[0] or possibly an alias
	   passed on the command line. */
	if (use_arg6_as_alias && argc >= 7)
	{
		ma.name = argv[6];
		ma.user_argc = argc - 7;
		ma.user_argv = &(argv[7]);
	}
	else
	{
		char *p = strrchr(argv[0], '/');
		if (p == NULL)
		{
			ma.name = argv[0];
		}
		else
		{
			ma.name = ++p;
		}
		ma.user_argc = argc - 6;
		ma.user_argv = &(argv[6]);
	}

	ma.namelen=strlen(ma.name);

	if (ma.user_argc == 0)
	{
		ma.user_argv = NULL;
	}

	/* File descriptors for the pipes */
	ma.to_fvwm = atoi(argv[1]);
	ma.from_fvwm = atoi(argv[2]);

	/* Ignore argv[3] */

	/* These two are generated as long hex strings */
	ma.window = strtoul(argv[4], NULL, 16);
	ma.decoration = strtoul(argv[5], NULL, 16);

	return &ma;
}

/* expands certain variables in a command to be sent by a module */
char *module_expand_action(
	Display *dpy, int screen , char *in_action, rectangle *r,
	char *forecolor, char *backcolor)
{
	char *variables[] =
	{
		"$",
		"fg",
		"bg",
		"left",
		"-left",
		"right",
		"-right",
		"top",
		"-top",
		"bottom",
		"-bottom",
		"width",
		"height",
		NULL
	};
	char *action = NULL;
	char *src;
	char *dest;
	char *string = NULL;
	char *rest;
	int val = 0;
	int offset;
	int i;
	char *dest_org;
	Bool is_string;
	Bool is_value;
	Bool has_geom;
	Bool has_fg;
	Bool has_bg;
	rectangle tmpr = { 0, 0, 0, 0 };

	has_geom = (r == NULL) ? False : True;
	has_fg = (forecolor == NULL) ? False : True;
	has_bg = (backcolor == NULL) ? False : True;
	if (r == NULL)
	{
		r = &tmpr;
	}
	/* create a temporary storage for expanding */
	action = xmalloc(MAX_MODULE_INPUT_TEXT_LEN);
	for (src = in_action, dest = action; *src != 0; src++)
	{
		if (*src != '$')
		{
			*(dest++) = *src;
			continue;
		}
		/* it's a variable */
		dest_org = dest;
		is_string = False;
		is_value = False;
		*(dest++) = *(src++);
		i = GetTokenIndex(src, variables, -1, &rest);
		if (i == -1)
		{
			src--;
			continue;
		}
		switch (i)
		{
		case 0: /* $ */
			continue;
		case 1: /* fg */
			string = forecolor;
			is_string = has_fg;
			break;
		case 2: /* bg */
			if (backcolor == NULL)
			{
				continue;
			}
			string = backcolor;
			is_string = has_bg;
			break;
		case 3: /* left */
			val = r->x;
			is_value = has_geom;
			break;
		case 4: /* -left */
			val = DisplayWidth(dpy, screen) - r->x - 1;
			is_value = has_geom;
			break;
		case 5: /* right */
			val = r->x + r->width;
			is_value = has_geom;
			break;
		case 6: /* -right */
			val = DisplayWidth(dpy, screen) - r->x - r->width - 1;
			is_value = has_geom;
			break;
		case 7: /* top */
			val = r->y;
			is_value = has_geom;
			break;
		case 8: /* -top */
			val = DisplayHeight(dpy, screen) - r->y - 1;
			is_value = has_geom;
			break;
		case 9: /* bottom */
			val = r->y + r->height;
			is_value = has_geom;
			break;
		case 10: /* -bottom */
			val = DisplayHeight(dpy, screen) - r->y - r->height - 1;
			is_value = has_geom;
			break;
		case 11: /* width */
			val = r->width;
			is_value = has_geom;
			break;
		case 12: /* height */
			val = r->height;
			is_value = has_geom;
			break;
		default: /* unknown */
			src--;
			continue;
		} /* switch */
		if (is_value == False && is_string == False)
		{
			src--;
			continue;
		}
		dest = dest_org;
		src = --rest;
		if (is_value)
		{
			if (MAX_MODULE_INPUT_TEXT_LEN - (dest - action) <= 16)
			{
				/* out of space */
				free(action);
				return NULL;
			}
			/* print the number into the string */
			sprintf(dest, "%d%n", val, &offset);
			dest += offset;
		}
		else if (is_string)
		{
			if (MAX_MODULE_INPUT_TEXT_LEN - (dest - action) <=
			    strlen(string))
			{
				/* out of space */
				free(action);
				return NULL;
			}
			/* print the colour name into the string */
			if (string)
			{
				sprintf(dest, "%s%n", string, &offset);
				dest += offset;
			}
		}
	} /* for */
	*dest = 0;

	return action;
}
