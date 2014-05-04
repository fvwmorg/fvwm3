/* -*-c-*- */
/* vim: set ts=8 shiftwidth=8: */
/* This module, FvwmProxy, is an original work by Jason Weber.
 *
 * This program is free software; you can redistribute it and/or modify
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


/* ---------------------------- included header files ----------------------- */

#include "config.h"
#include <stdio.h>
#include <limits.h>

#include "libs/Module.h"
#include "libs/fvwmlib.h"
#include "libs/FRenderInit.h"
#include "libs/FRender.h"
#include "libs/Colorset.h"
#include "libs/Flocale.h"
#include "libs/gravity.h"
#include "libs/FScreen.h"
#include "libs/Picture.h"
#include "libs/PictureGraphics.h"
#include "libs/charmap.h"
#include "libs/modifiers.h"
#include "libs/ColorUtils.h"
#include "libs/Graphics.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/System.h"
#include "libs/wild.h"

#include "FvwmProxy.h"

/* ---------------------------- local definitions --------------------------- */

#define PROXY_COMMAND_DEBUG	False
#define PROXY_GROUP_DEBUG	False
#define PROXY_EVENT_DEBUG	False

#define PROXY_KEY_POLLING	True

/* defaults for things we put in a configuration file */

#define PROXY_WIDTH		180
#define PROXY_HEIGHT		60
#define PROXY_SEPARATION	10
#define PROXY_MINWIDTH		15
#define PROXY_MINHEIGHT		10
#define PROXY_SLOT_SIZE		16
#define PROXY_SLOT_SPACE	4
#define PROXY_GROUP_SLOT	2
#define PROXY_GROUP_COUNT	6
#define PROXY_STAMP_LIMIT	8

#define STARTUP_DEBUG		False	/* store output before log is opened */

#define CMD_SELECT		"WindowListFunc $[w.id]"
#define CMD_CLICK1		"Raise"
#define CMD_CLICK3		"Lower"
#define CMD_DEFAULT		"Nop"

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static char *ImagePath = NULL;
static char *MyName;
static int x_fd;
static fd_set_size_t fd_width;
static int fd[2];
static Display *dpy;
static unsigned long screen;
static GC fg_gc;
static GC hi_gc;
static GC sh_gc;
static GC miniIconGC;
static Window rootWindow;
static Window focusWindow;
static XTextProperty windowName;
static int deskNumber=0;
static int mousex,mousey;
static ProxyWindow *firstProxy=NULL;
static ProxyWindow *lastProxy=NULL;
static ProxyWindow *selectProxy=NULL;
static ProxyWindow *startProxy=NULL;
static ProxyWindow *enterProxy=NULL;
static ProxyWindow *last_rotation_instigator=NULL;
static ProxyGroup *firstProxyGroup=NULL;
static FvwmPicture **pictureArray=NULL;
static int numSlots=0;
static int miniIconSlot=0;
static XGCValues xgcv;
static int are_windows_shown = 0;
static int watching_modifiers = 0;
static int waiting_to_config = 0;
static int waiting_to_stamp = 0;
static int pending_do = 0;
static unsigned int held_modifiers=0;
static int watched_modifiers=0;
static FlocaleWinString *FwinString;

static int cset_normal = 0;
static int cset_select = 0;
static int cset_iconified = 0;
static char *font_name = NULL;
static char *small_font_name = NULL;
static FlocaleFont *Ffont;
static FlocaleFont *Ffont_small;
static int enterSelect=False;
static int showMiniIcons=True;
static int proxyIconified=False;
static int proxyMove=False;
static int proxyWidth=PROXY_WIDTH;
static int proxyHeight=PROXY_HEIGHT;
static int proxySeparation=PROXY_SEPARATION;
static int slotWidth=PROXY_SLOT_SIZE;
static int slotHeight=PROXY_SLOT_SIZE;
static int slotSpace=PROXY_SLOT_SPACE;
static int groupSlot=PROXY_GROUP_SLOT;
static int groupCount=PROXY_GROUP_COUNT;
static int stampLimit=PROXY_STAMP_LIMIT;

static GeometryStamp* stampQueue=NULL;
static int stamp= 0;
static int stampMin=0;
static int stampMax=0;

static char commandBuffer[256];
static char resultBuffer[256];

typedef enum
{
	PROXY_ACTION_SELECT = 0,
	PROXY_ACTION_SHOW,
	PROXY_ACTION_HIDE,
	PROXY_ACTION_ABORT,
	PROXY_ACTION_MARK,
	PROXY_ACTION_UNMARK,
	PROXY_ACTION_MODIFIER_RELEASE,
	/* this one *must* be last */
	PROXY_ACTION_CLICK,
	PROXY_ACTION_LAST = PROXY_ACTION_CLICK +
	NUMBER_OF_EXTENDED_MOUSE_BUTTONS
} proxy_action_t;

typedef enum
{
	PROXY_FOLLOW_OFF = 0,
	PROXY_FOLLOW_ON,
	PROXY_FOLLOW_INHERIT
} proxy_follow_t;

typedef enum
{
	PROXY_PROVOKE_RAISE = 1,
	PROXY_PROVOKE_DESK,
	PROXY_PROVOKE_DRAG,
	PROXY_PROVOKE_ICON
} proxy_provoke_t;

typedef enum
{
	PROXY_SHOWONLY_ALL = 0,
	PROXY_SHOWONLY_SELECT,
	PROXY_SHOWONLY_COVER,
	PROXY_SHOWONLY_GROUP
} proxy_showonly_t;
static proxy_showonly_t showOnly = PROXY_SHOWONLY_ALL;

#define GROUP_COLORS 7
static char* group_color[GROUP_COLORS][2] =
{
	{"#dddddd",	"#777777"},
	{"#ffcccc",	"#ff0000"},
	{"#ccffcc",	"#00ff00"},
	{"#ccccff",	"#0000ff"},
	{"#ffccff",	"#ff00ff"},
	{"#ffffcc",	"#ffff00"},
	{"#ccffff",	"#00ffff"}
};

char *action_list[PROXY_ACTION_LAST];
char **slot_action_list[NUMBER_OF_EXTENDED_MOUSE_BUTTONS];

static WindowName* new_WindowName(void);
static ProxyGroup* FindProxyGroup(char* groupname);

static int (*originalXErrorHandler)(Display *,XErrorEvent *);
static int (*originalXIOErrorHandler)(Display *);

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions (options) ------------------- */

static void ExpandSlots(int slots)
{
	int m;
	int n;
	if(slots>numSlots)
	{
		pictureArray=(FvwmPicture**)realloc(pictureArray,
			sizeof(FvwmPicture*)*(slots));
		for(m=0;m<NUMBER_OF_EXTENDED_MOUSE_BUTTONS;m++)
		{
			slot_action_list[m]=(char**)realloc(
				slot_action_list[m],
				sizeof(char*)*(slots));
			for(n=numSlots;n<slots;n++)
			{
				slot_action_list[m][n]=NULL;
			}
		}
		for(n=numSlots;n<slots;n++)
		{
			pictureArray[n]=NULL;
		}
		numSlots=slots;
	}
}

static void LinkSlotAction(char *string)
{
	char *token;
	int slot;

	token = PeekToken(string, &string);
	slot=atoi(token);

	token = PeekToken(string, &string);
	if (strncasecmp(token, "Click", 5) == 0)
	{
		int b;
		int i;
		i = sscanf(token + 5, "%d", &b);
		if (i > 0 && b >=1 && b <= NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
		{
			ExpandSlots(slot+1);
			if (slot_action_list[b-1][slot] != NULL)
			{
				free(slot_action_list[b-1][slot]);
			}
			slot_action_list[b-1][slot] = xstrdup(string);
#if PROXY_GROUP_DEBUG
			fprintf(stderr,"slot_action_list[%d][%d]=\"%s\"\n",
				b-1,slot,slot_action_list[b-1][slot]);
#endif
		}
	}
}

static WindowName* FindWindowName(WindowName* namelist,char* name)
{
	WindowName* include=namelist;
	while(include)
	{
		if(matchWildcards(include->name, name))
		{
			return include;
		}
		include=include->next;
	}

	return NULL;
}

static void LinkAction(char *string)
{
	char *token;

	token = PeekToken(string, &string);
	if (strncasecmp(token, "Click", 5) == 0)
	{
		int b;
		int i;
		i = sscanf(token + 5, "%d", &b);
		if (i > 0 && b >=1 && b <= NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
		{
			if (action_list[PROXY_ACTION_CLICK + b - 1] != NULL)
			{
				free(action_list[PROXY_ACTION_CLICK + b - 1]);
			}
			action_list[PROXY_ACTION_CLICK + b - 1] =
				xstrdup(string);
		}
	}
	else if (StrEquals(token, "Select"))
	{
		if (action_list[PROXY_ACTION_SELECT] != NULL)
		{
			free(action_list[PROXY_ACTION_SELECT]);
		}
		action_list[PROXY_ACTION_SELECT] = xstrdup(string);
	}
	else if (StrEquals(token, "Show"))
	{
		if (action_list[PROXY_ACTION_SHOW] != NULL)
		{
			free(action_list[PROXY_ACTION_SHOW]);
		}
		action_list[PROXY_ACTION_SHOW] = xstrdup(string);
	}
	else if (StrEquals(token, "Hide"))
	{
		if (action_list[PROXY_ACTION_HIDE] != NULL)
		{
			free(action_list[PROXY_ACTION_HIDE]);
		}
		action_list[PROXY_ACTION_HIDE] = xstrdup(string);
	}
	else if (StrEquals(token, "Abort"))
	{
		if (action_list[PROXY_ACTION_ABORT] != NULL)
		{
			free(action_list[PROXY_ACTION_ABORT]);
		}
		action_list[PROXY_ACTION_ABORT] = xstrdup(string);
	}
	else if (StrEquals(token, "Mark"))
	{
		if (action_list[PROXY_ACTION_MARK] != NULL)
		{
			free(action_list[PROXY_ACTION_MARK]);
		}
		action_list[PROXY_ACTION_MARK] = xstrdup(string);
	}
	else if (StrEquals(token, "Unmark"))
	{
		if (action_list[PROXY_ACTION_UNMARK] != NULL)
		{
			free(action_list[PROXY_ACTION_UNMARK]);
		}
		action_list[PROXY_ACTION_UNMARK] = xstrdup(string);
	}
	else if (StrEquals(token, "ModifierRelease"))
	{
		token = PeekToken(string, &string);

		if (action_list[PROXY_ACTION_MODIFIER_RELEASE] != NULL)
		{
			free(action_list[PROXY_ACTION_MODIFIER_RELEASE]);
		}

		modifiers_string_to_modmask(token, (int *)&watched_modifiers);
		action_list[PROXY_ACTION_MODIFIER_RELEASE] = xstrdup(string);
	}

	return;
}

#if 0
static void parse_cmd(char **ret_cmd, char *cmd)
{
	if (*ret_cmd != NULL)
	{
		free(*ret_cmd);
		*ret_cmd = NULL;
	}
	if (cmd != NULL)
	{
		*ret_cmd = safestrdup(cmd);
	}

	return;
}
#endif

FvwmPicture* loadPicture(char* name)
{
	FvwmPicture *picture=NULL;
	FvwmPictureAttributes fpa;

	if(!name)
	{
		return NULL;
	}

	fpa.mask = FPAM_NO_COLOR_LIMIT;
	picture = PGetFvwmPicture(dpy, RootWindow(dpy,screen),
		ImagePath, name, fpa);
	if (!picture)
	{
		fprintf(stderr, "loadPixmap failed to load \"%s\"\n", name);
	}
	return picture;
}

static Bool parse_options(void)
{
	int m;
	char *tline;

	memset(action_list, 0, sizeof(action_list));
	action_list[PROXY_ACTION_SELECT] = strdup(CMD_SELECT);
	action_list[PROXY_ACTION_CLICK + 0] = strdup(CMD_CLICK1);
	if (NUMBER_OF_EXTENDED_MOUSE_BUTTONS > 2)
	{
		action_list[PROXY_ACTION_CLICK + 2] = strdup(CMD_CLICK3);
	}
	for (m=0;m<PROXY_ACTION_LAST;m++)
	{
		if (action_list[m]==NULL)
		{
			action_list[m] = strdup(CMD_DEFAULT);
		}
	}

	InitGetConfigLine(fd, CatString3("*", MyName, 0));
	for (GetConfigLine(fd, &tline); tline != NULL;
		GetConfigLine(fd, &tline))
	{
		char *resource;
		char *token;
		char *next;

		token = PeekToken(tline, &next);
		if (StrEquals(token, "Colorset"))
		{
			LoadColorset(next);
			continue;
		}
		if (StrEquals(token, "ImagePath"))
		{
			if (ImagePath)
			{
				free(ImagePath);
			}
			CopyString(&ImagePath, next);
			continue;
		}

		tline = GetModuleResource(tline, &resource, MyName);
		if (resource == NULL)
		{
			continue;
		}

		/* dump leading whitespace */
		while (*tline==' ' || *tline=='\t')
			tline++;
		if (!strncasecmp(resource,"Action",6))
		{
			LinkAction(tline);
		}
		else if (!strncasecmp(resource,"SlotAction",10))
		{
			LinkSlotAction(tline);
		}
		else if (StrEquals(resource, "Colorset"))
		{
			if (sscanf(tline, "%d", &cset_normal) < 1)
			{
				cset_normal = 0;
			}
		}
		else if (StrEquals(resource, "SelectColorset"))
		{
			if (sscanf(tline, "%d", &cset_select) < 1)
			{
				cset_select = 0;
			}
		}
		else if (StrEquals(resource, "IconifiedColorset"))
		{
			if (sscanf(tline, "%d", &cset_iconified) < 1)
			{
				cset_iconified = 0;
			}
		}
		else if (StrEquals(resource, "Font"))
		{
			if (font_name != NULL)
			{
				free(font_name);
			}
			font_name = xstrdup(tline);
		}
		else if (StrEquals(resource, "SmallFont"))
		{
			if (small_font_name != NULL)
			{
				free(small_font_name);
			}
			small_font_name = xstrdup(tline);
		}
		else if (StrEquals(resource, "ShowMiniIcons"))
		{
			showMiniIcons = ParseToggleArgument(tline, NULL, 0, 1);
		}
		else if (StrEquals(resource, "EnterSelect"))
		{
			enterSelect = ParseToggleArgument(tline, NULL, 0, 1);
		}
		else if (StrEquals(resource, "ProxyMove"))
		{
			proxyMove = ParseToggleArgument(tline, NULL, 0, 1);
		}
		else if (StrEquals(resource, "ProxyIconified"))
		{
			proxyIconified = ParseToggleArgument(tline, NULL, 0, 1);
		}
		else if (StrEquals(resource, "Width"))
		{
			if (sscanf(tline, "%d", &proxyWidth) < 1)
				proxyWidth=PROXY_MINWIDTH;
		}
		else if (StrEquals(resource, "Height"))
		{
			if (sscanf(tline, "%d", &proxyHeight) < 1)
				proxyHeight=PROXY_MINHEIGHT;
		}
		else if (StrEquals(resource, "Separation"))
		{
			if (sscanf(tline, "%d", &proxySeparation) < 1)
				proxySeparation=False;
		}
		else if (StrEquals(resource, "UndoLimit"))
		{
			if (sscanf(tline, "%d", &stampLimit) < 1)
				stampLimit=PROXY_STAMP_LIMIT;
		}
		else if (StrEquals(resource, "ShowOnly"))
		{
#if PROXY_GROUP_DEBUG
			fprintf(stderr, "ShowOnly: \"%s\"\n", tline);
#endif
			if(StrEquals(tline, "All"))
			{
				showOnly = PROXY_SHOWONLY_ALL;
			}
			else if(StrEquals(tline, "Selected"))
			{
				showOnly = PROXY_SHOWONLY_SELECT;
			}
			else if(StrEquals(tline, "Covered"))
			{
				showOnly = PROXY_SHOWONLY_COVER;
			}
			else if(StrEquals(tline, "Grouped"))
			{
				showOnly = PROXY_SHOWONLY_GROUP;
			}
		}
		else if (StrEquals(resource, "SlotWidth"))
		{
			if (sscanf(tline, "%d", &slotWidth) < 1)
				slotWidth=PROXY_SLOT_SIZE;
		}
		else if (StrEquals(resource, "SlotHeight"))
		{
			if (sscanf(tline, "%d", &slotHeight) < 1)
				slotHeight=PROXY_SLOT_SIZE;
		}
		else if (StrEquals(resource, "SlotSpace"))
		{
			if (sscanf(tline, "%d", &slotSpace) < 1)
				slotSpace=PROXY_SLOT_SPACE;
		}
		else if (StrEquals(resource, "GroupSlot"))
		{
			if (sscanf(tline, "%d", &groupSlot) < 0)
				groupSlot=PROXY_GROUP_SLOT;
		}
		else if (StrEquals(resource, "GroupCount"))
		{
			if (sscanf(tline, "%d", &groupCount) < 0)
				groupCount=PROXY_GROUP_COUNT;
		}
		else if (StrEquals(resource, "SlotStyle"))
		{
			int slot= -1;
			char style[128];
			char name[128];
			int bytes=0;
			int args = sscanf(tline, "%d%s%*[^\"]\"%[^\"]\"%n",
				&slot,style,name,&bytes);
#if PROXY_GROUP_DEBUG
			fprintf(stderr,
				"SlotStyle: %d \"%s\" -> %d \"%s\" \"%s\"\n",
				args,tline,slot,style,name);
#endif
			if (args>=3 && StrEquals(style, "Pixmap"))
			{
				ExpandSlots(slot+1);
				pictureArray[slot]=loadPicture(name);
			}
			else if (args>=2 && StrEquals(style, "MiniIcon"))
			{
				miniIconSlot=slot;
			}
		}
		else if (StrEquals(resource, "Group"))
		{
			ProxyGroup* proxygroup;
			char groupname[128];
			char directive[128];
			char tail[128];
			char pattern[128];
			int bytes=0;
			(void)sscanf(tline, "\"%[^\"]\"%s%n",
				groupname,directive,&bytes);

			strncpy(tail,&tline[bytes],128);
			tail[127]=0;
			pattern[0]=0;
			(void)sscanf(tail, "%*[^\"]\"%[^\"]\"",pattern);

#if PROXY_GROUP_DEBUG
			fprintf(stderr,
				"Group: \"%s\" -> \"%s\" \"%s\" \"%s\"\n",
				tline,groupname,directive,pattern);
#endif

			proxygroup=FindProxyGroup(groupname);
			if (StrEquals(directive, "IgnoreIDs"))
			{
				proxygroup->flags.ignore_ids=1;
			}
			else if (StrEquals(directive, "AutoInclude"))
			{
				proxygroup->flags.auto_include=1;
			}
			else if (StrEquals(directive, "AutoSoft"))
			{
				proxygroup->flags.auto_include=1;
				proxygroup->flags.auto_soft=1;
			}
			else if (StrEquals(directive, "Isolated"))
			{
				proxygroup->flags.isolated=1;
			}
			else if (StrEquals(directive, "Include") ||
				StrEquals(directive, "WeakInclude") ||
				StrEquals(directive, "SoftInclude") ||
				StrEquals(directive, "WeakSoftInclude"))
			{
				WindowName* include=new_WindowName();
				include->name=strdup(pattern);
				include->flags.is_soft=
					(StrEquals(directive,
					"SoftInclude") ||
					StrEquals(directive,
					"WeakSoftInclude"));
				include->flags.is_weak=
					(StrEquals(directive,
					"WeakInclude") ||
					StrEquals(directive,
					"WeakSoftInclude"));
				include->next=proxygroup->includes;
				proxygroup->includes=include;
#if PROXY_GROUP_DEBUG
				fprintf(stderr,"Include \"%s\"\n",
					pattern);
#endif
			}
			else if (StrEquals(directive, "Exclude"))
			{
				WindowName* exclude=new_WindowName();
				exclude->name=strdup(pattern);
				exclude->next=proxygroup->excludes;
				proxygroup->excludes=exclude;
#if PROXY_GROUP_DEBUG
				fprintf(stderr,"Exclude \"%s\"\n",
					pattern);
#endif
			}
			else
			{
/*
			No Soft/Hard Raise/Desk/Drag
*/
				char* remainder = directive;
				int soft;
				int hard;
				int raise;
				int desk;
				int drag;
				int icon;

				proxy_follow_t setting = PROXY_FOLLOW_ON;
				if(StrHasPrefix(remainder,"No"))
				{
					setting = PROXY_FOLLOW_OFF;
					remainder += 2;
				}
				if(StrHasPrefix(remainder,"Inherit"))
				{
					setting = PROXY_FOLLOW_INHERIT;
					remainder += 7;
				}

				soft = True;
				hard = True;
				if(StrHasPrefix(remainder,"Soft"))
				{
					hard = False;
					remainder += 4;
				}
				if(StrHasPrefix(remainder,"Hard"))
				{
					soft = False;
					remainder += 4;
				}

				raise = True;
				desk = True;
				drag = True;
				icon = True;
				if(StrHasPrefix(remainder,"Raise"))
				{
					desk = False;
					drag = False;
					icon = False;
					remainder += 5;
				}
				if(StrHasPrefix(remainder,"Desk"))
				{
					raise = False;
					drag = False;
					icon = False;
					remainder += 4;
				}
				if(StrHasPrefix(remainder,"Drag"))
				{
					raise = False;
					desk = False;
					icon = False;
					remainder += 4;
				}
				if(StrHasPrefix(remainder,"Icon"))
				{
					raise = False;
					desk = False;
					drag = False;
					remainder += 4;
				}
				if(StrHasPrefix(remainder,"All"))
				{
					remainder += 3;
				}
#if PROXY_GROUP_DEBUG
				fprintf(stderr,"@@@ ");
				fprintf(stderr,"Follow \"%s\"\n",
					pattern);
				fprintf(stderr,"@@@ ");
				fprintf(stderr,"  setting %d soft %d hard %d\n",
					setting, soft, hard);
				fprintf(stderr,"@@@ ");
				fprintf(stderr,"  raise %d desk %d",
					raise, desk);
				fprintf(stderr," drag %d icon %d\n",
					drag, icon);
#endif

				if(pattern[0])
				{
					WindowName* windowName = FindWindowName(
						proxygroup->includes, pattern);
					if(windowName)
					{
						if(soft)
						{
							if(raise) windowName->
								flags.
								soft_raise =
								setting;
							if(desk) windowName->
								flags.
								soft_desk =
								setting;
							if(drag) windowName->
								flags.
								soft_drag =
								setting;
							if(icon) windowName->
								flags.
								soft_icon =
								setting;
						}
						if(hard)
						{
							if(raise) windowName->
								flags.
								hard_raise =
								setting;
							if(desk) windowName->
								flags.
								hard_desk =
								setting;
							if(drag) windowName->
								flags.
								hard_drag =
								setting;
							if(icon) windowName->
								flags.
								hard_icon =
								setting;
						}
					}
				}
				else
				{
					if(soft)
					{
						if(raise) proxygroup->
							flags.soft_raise =
							setting;
						if(desk) proxygroup->
							flags.soft_desk =
							setting;
						if(drag) proxygroup->
							flags.soft_drag =
							setting;
						if(icon) proxygroup->
							flags.soft_icon =
							setting;
					}
					if(hard)
					{
						if(raise) proxygroup->
							flags.hard_raise =
							setting;
						if(desk) proxygroup->
							flags.hard_desk =
							setting;
						if(drag) proxygroup->
							flags.hard_drag =
							setting;
						if(icon) proxygroup->
							flags.hard_icon =
							setting;
					}
				}
			}
		}
		else
		{
			fprintf(stderr,"Unknown: \"%s\"\n",tline);
		}

		free(resource);
	}

	return True;
}

/* ---------------------------- local functions (classes) ------------------- */

/* classes */

static void ProxyWindow_ProxyWindow(ProxyWindow *p)
{
	memset(p, 0, sizeof *p);
}

static ProxyWindow *new_ProxyWindow(void)
{
	ProxyWindow *p=xmalloc(sizeof(ProxyWindow));
	ProxyWindow_ProxyWindow(p);
	return p;
}

static void delete_ProxyWindow(ProxyWindow *p)
{
	if (p)
	{
		if (p->name)
		{
			free(p->name);
		}
		if (p->iconname)
		{
			free(p->iconname);
		}
		if (p->proxy != None)
		{
			XDestroyWindow(dpy, p->proxy);
		}
		free(p);
	}
}

static void WindowName_WindowName(WindowName *p)
{
	memset(p, 0, sizeof *p);

	p->flags.soft_raise = PROXY_FOLLOW_INHERIT;
	p->flags.soft_desk = PROXY_FOLLOW_INHERIT;
	p->flags.soft_drag = PROXY_FOLLOW_INHERIT;
	p->flags.soft_icon = PROXY_FOLLOW_INHERIT;
	p->flags.hard_raise = PROXY_FOLLOW_INHERIT;
	p->flags.hard_desk = PROXY_FOLLOW_INHERIT;
	p->flags.hard_drag = PROXY_FOLLOW_INHERIT;
	p->flags.hard_icon = PROXY_FOLLOW_INHERIT;
}

static WindowName *new_WindowName(void)
{
	WindowName *p=xmalloc(sizeof(WindowName));
	WindowName_WindowName(p);
	return p;
}

static void ProxyGroup_ProxyGroup(ProxyGroup *p)
{
	memset(p, 0, sizeof *p);

	p->flags.soft_raise = PROXY_FOLLOW_ON;
	p->flags.soft_desk = PROXY_FOLLOW_ON;
	p->flags.soft_drag = PROXY_FOLLOW_ON;
	p->flags.soft_icon = PROXY_FOLLOW_ON;
	p->flags.hard_raise = PROXY_FOLLOW_ON;
	p->flags.hard_desk = PROXY_FOLLOW_ON;
	p->flags.hard_drag = PROXY_FOLLOW_ON;
	p->flags.hard_icon = PROXY_FOLLOW_ON;
}

static ProxyGroup *new_ProxyGroup(void)
{
	ProxyGroup *p=xmalloc(sizeof(ProxyGroup));
	ProxyGroup_ProxyGroup(p);
	return p;
}

/* ---------------------------- error handlers ------------------------------ */

static int myXErrorHandler(Display *display,XErrorEvent *error_event)
{
	const long messagelen=256;
	char buffer[messagelen],function[messagelen];
	char request_number[16];

	sprintf(request_number,"%d",error_event->request_code);
	sprintf(buffer,"UNKNOWN");
	XGetErrorDatabaseText(display,"XRequest",
		request_number,buffer,function,messagelen);

	fprintf(stderr, "non-fatal X error as follows, display %p"
		" op %d:%d \"%s\" serial %u error %d\n",
		display,
		error_event->request_code,error_event->minor_code,
		function,(unsigned int)error_event->serial,
		error_event->error_code);

	return 0;
}

static int myXIOErrorHandler(Display *display)
{
	fprintf(stderr, "fatal IO Error on display %p\n", display);
	originalXIOErrorHandler(display);

	/* should never get this far */
	return 0;
}

/* ---------------------------- local functions ----------------------------- */

static void send_command_to_fvwm(char *command, Window w)
{
	if (command == NULL || *command == 0)
	{
		return;
	}
#if PROXY_COMMAND_DEBUG
	fprintf(stderr,"SendText: \"%s\"\n", command);
#endif
	SendText(fd, command, w);

	return;
}

static int GetProperty(Window w,char* propertyname)
{
	Atom atom,actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char *prop;
	int status;
	int result=0;
	int byte;
	int bytes;

	atom = XInternAtom(dpy, propertyname, True);

	status = XGetWindowProperty(dpy, w, atom, 0L, 1024,
		False, AnyPropertyType,
		&actual_type,
		&actual_format, &nitems,
		&bytes_after,
		&prop);
	if (status!=0)
	{
/*		fprintf(stderr,"GetProperty: cannot get %s\n",propertyname);
*/
		return 0;
	}
	if (!prop) {
/*		fprintf(stderr,"GetProperty: no properties\n");
*/
		return 0;
	}

	bytes=actual_format/8;
	for(byte=bytes-1;byte>=0;byte--)
	{
		result=result*256+prop[byte];
	}
	XFree(prop);
	return result;
}

static int GetProcessId(Window w)
{
	return GetProperty(w, "_NET_WM_PID");
}

static int GetLeader(Window w)
{
	int result=GetProperty(w, "WM_CLIENT_LEADER");
	if(!result)
	{
		XWMHints* hints=XGetWMHints(dpy,w);
		if(hints && hints->flags&WindowGroupHint)
		{
			result=hints->window_group;
			XFree(hints);
		}
	}
	return result;
}

static int GetParentProcessId(int pid)
{
	int ppid=0;
	FILE* statusfile;

	sprintf(commandBuffer,"/proc/%d/stat",pid);
	statusfile=fopen(commandBuffer,"r");
	if(!statusfile)
	{
		return 0;
	}
	{
		int n;
		n = fread(resultBuffer,32,1,statusfile);
		(void)n;
	}
	sscanf(resultBuffer,"%*d %*[^)]) %*s %d",&ppid);
	fclose(statusfile);
	return ppid;
}

static ProxyGroup* FindProxyGroup(char* groupname)
{
	ProxyGroup* proxygroup=firstProxyGroup;
	while(proxygroup)
	{
		if(StrEquals(proxygroup->name, groupname))
		{
			return proxygroup;
		}
		proxygroup=proxygroup->next;
	}
	proxygroup=new_ProxyGroup();
	proxygroup->next=firstProxyGroup;
	proxygroup->name=strdup(groupname);
	firstProxyGroup=proxygroup;
	return proxygroup;
}

static int MatchWindowName(WindowName* namelist,char* name)
{
	WindowName* include=FindWindowName(namelist, name);
	return (include != NULL);
}

static ProxyGroup* FindProxyGroupWithWindowName(char* name)
{
	ProxyGroup* proxygroup=firstProxyGroup;
#if PROXY_GROUP_DEBUG
	fprintf(stderr,"FindProxyGroupWithWindowName(%s)\n",name);
#endif

	while(proxygroup)
	{
		if(MatchWindowName(proxygroup->includes, name))
		{
#if PROXY_GROUP_DEBUG
			fprintf(stderr," found in %s\n",proxygroup->name);
#endif
			return proxygroup;
		}
		proxygroup=proxygroup->next;
	}

	return NULL;
}

static ProxyGroup* FindProxyGroupOfNeighbor(ProxyWindow* instigator)
{
	ProxyWindow *proxy;

	if(!instigator->group)
	{
		return NULL;
	}

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(proxy==instigator)
		{
			continue;
		}
		if (proxy->desk == deskNumber &&
			proxy->group==instigator->group && proxy->proxy_group)
		{
			return proxy->proxy_group;
		}
	}
	return NULL;
}

static ProxyWindow *FindProxy(Window window)
{
	ProxyWindow *proxy=firstProxy;

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if (proxy->proxy==window || proxy->window==window)
		{
			return proxy;
		}
	}

	return NULL;
}

static void DrawPicture(
	Window window, int x, int y, FvwmPicture *picture, int cset)
{
	FvwmRenderAttributes fra;

	if (!picture || picture->picture == None)
		return;

	fra.mask = FRAM_DEST_IS_A_WINDOW;
	if (cset >= 0)
	{
		fra.mask |= FRAM_HAVE_ICON_CSET;
		fra.colorset = &Colorset[cset];
	}
	PGraphicsRenderPicture(
		dpy, window, picture, &fra, window, miniIconGC, None, None,
		0, 0, picture->width, picture->height,
		x, y, picture->width, picture->height, False);
}

static void DrawWindow(
	ProxyWindow *proxy, int x,int y,int w,int h)
{
	int texty,maxy;
	int cset;
	FvwmPicture *picture = &proxy->picture;
	int drawMiniIcon=(showMiniIcons && proxy->picture.picture != None);
	int group;
	int m;
	char *big_name;
	char *small_name;
	int overrun=0;

	if (!proxy || proxy->proxy == None)
	{
		return;
	}

	x=0;
	y=0;
	w=proxy->proxyw;
	h=proxy->proxyh;

	texty=(h+ Ffont->ascent - Ffont->descent)/2;	/* center */
	if (drawMiniIcon)
	{
		texty+=4;
	}

	maxy=h-Ffont->descent-4;
	if (texty>maxy)
	{
		texty=maxy;
	}

	cset = (proxy==selectProxy) ? cset_select :
		((proxy->flags.is_iconified) ? cset_iconified : cset_normal);
	XSetForeground(dpy,fg_gc,Colorset[cset].fg);
	XSetBackground(dpy,fg_gc,Colorset[cset].bg);
	XSetForeground(dpy,sh_gc,Colorset[cset].shadow);
	XSetForeground(dpy,hi_gc,Colorset[cset].hilite);

	/* FIXME: use clip redrawing (not really essential here) */
	if (FLF_FONT_HAS_ALPHA(Ffont,cset) || PICTURE_HAS_ALPHA(picture,cset))
	{
		XClearWindow(dpy,proxy->proxy);
	}
	RelieveRectangle(dpy,proxy->proxy, 0,0, w - 1,h - 1, hi_gc,sh_gc, 2);

#if 0
	if (proxy->iconname != NULL)
	{
		free(proxy->iconname);
	}
	proxy->iconname = safestrdup("    ");
	sprintf(proxy->iconname, "%d", proxy->stack);
#endif

	big_name = proxy->iconname;
	if(big_name==NULL || !big_name[0])
	{
		big_name = proxy->name;
	}
	small_name = proxy->name;
	if(small_name == NULL)
	{
		small_name=big_name;
	}
	if (big_name != NULL && big_name[0])
	{
		int text_width = FlocaleTextWidth(
			Ffont,big_name,strlen(big_name));
		int edge=(w-text_width)/2;

		if (edge<5)
		{
			edge=5;
		}
		if(w-text_width<5)
		{
			overrun=1;
		}

		FwinString->str = big_name;
		FwinString->win = proxy->proxy;
		FwinString->x = edge;
		FwinString->y = texty;
		FwinString->gc = fg_gc;
		FwinString->flags.has_colorset = False;
		if (cset >= 0)
		{
			FwinString->colorset = &Colorset[cset];
			FwinString->flags.has_colorset = True;
		}
		FlocaleDrawString(dpy, Ffont, FwinString, 0);
	}
	if (small_name != NULL && small_name[0] &&
		(overrun || strcmp(small_name,big_name)) && Ffont_small!=NULL)
	{
		int text_width = FlocaleTextWidth(
			Ffont_small,small_name,strlen(small_name));
		int edge=(w-text_width)/2;

		if (edge<5)
		{
			edge=w-text_width-5;
		}

		FwinString->str = small_name;
		FwinString->win = proxy->proxy;
		FwinString->x = edge;
		FwinString->y = h-Ffont_small->descent-3;
		FwinString->gc = hi_gc;
		FwinString->flags.has_colorset = False;
		if (cset >= 0)
		{
			FwinString->colorset = &Colorset[cset];
			FwinString->flags.has_colorset = True;
		}
		FlocaleDrawString(dpy, Ffont_small, FwinString, 0);
	}
	if (drawMiniIcon && miniIconSlot>0)
	{
		int widgetx=slotSpace+(slotWidth+slotSpace)*(miniIconSlot-1);
		DrawPicture(proxy->proxy, widgetx, slotSpace, picture, cset);
	}
	for(group=1;group<groupCount+1;group++)
	{
		int lit=(proxy->group==group);
		int widgetx=slotSpace+
			(slotWidth+slotSpace)*(groupSlot+group-2);
		int color_index=group%GROUP_COLORS;
		int drawsoft=lit && proxy->flags.is_soft;
		int drawisolated=lit && proxy->flags.is_isolated;

		if(drawsoft)
		{
			XSetForeground(dpy,fg_gc,GetColor(
				group_color[color_index][0]));
			XFillRectangle(dpy,proxy->proxy,fg_gc,
				widgetx,slotSpace,
				slotWidth,slotHeight);

			XSetForeground(dpy,sh_gc,GetColor("black"));
			RelieveRectangle(dpy,proxy->proxy,
				widgetx,slotSpace,
				slotWidth,slotHeight/2,
				hi_gc,sh_gc, 1);
		}
		XSetForeground(dpy,fg_gc,GetColor(
			group_color[color_index][proxy->group==group]));
		XFillRectangle(dpy,proxy->proxy,fg_gc,
			widgetx,slotSpace,
			slotWidth,
			drawsoft? slotHeight/2: slotHeight);

		XSetForeground(dpy,sh_gc,GetColor(lit? "black": "white"));
		XSetForeground(dpy,hi_gc,GetColor(lit? "black": "white"));
		RelieveRectangle(dpy,proxy->proxy,
			widgetx,slotSpace,
			slotWidth,slotHeight,
			hi_gc,sh_gc, 2);
		if(drawisolated)
		{
			RelieveRectangle(dpy,proxy->proxy,
				widgetx+4,slotSpace+4,
				slotWidth-8,slotHeight-8,
				hi_gc,sh_gc, 2);
		}
	}
	for(m=0;m<numSlots;m++)
	{
		int widgetx=slotSpace+(slotWidth+slotSpace)*(m-1);
		DrawPicture(proxy->proxy, widgetx, slotSpace, pictureArray[m],
			cset);
	}
}

static void DrawProxy(ProxyWindow *proxy)
{
	if (proxy)
	{
		DrawWindow(
			proxy, proxy->proxyx, proxy->proxyy, proxy->proxyw,
			proxy->proxyh);
	}
}

static void DrawProxyBackground(ProxyWindow *proxy)
{
	int cset;

	if (proxy == NULL || proxy->proxy == None)
	{
		return;
	}
	cset = (proxy==selectProxy) ? cset_select :
		((proxy->flags.is_iconified) ? cset_iconified : cset_normal);
	XSetForeground(dpy,fg_gc,Colorset[cset].fg);
	XSetBackground(dpy,fg_gc,Colorset[cset].bg);
	SetWindowBackground(
		dpy, proxy->proxy, proxy->proxyw, proxy->proxyh,
		&Colorset[cset], Pdepth, fg_gc, True);
}

static void OpenOneWindow(ProxyWindow *proxy)
{
	int border=0;
	unsigned long valuemask=CWOverrideRedirect;
	XSetWindowAttributes attributes;

	if (proxy == NULL ||
		proxy->desk != deskNumber ||
		(!proxyIconified && proxy->flags.is_iconified) ||
		proxy->flags.is_shown)
	{
		return;
	}
	if (showOnly)
	{
		if (!focusWindow)
		{
			return;
		}
		if ((!selectProxy || proxy != selectProxy) &&
			proxy->window != focusWindow)
		{
			ProxyWindow *other;
			if (showOnly == PROXY_SHOWONLY_SELECT)
			{
				return;
			}
			other = FindProxy(focusWindow);
#if PROXY_GROUP_DEBUG
			fprintf(stderr, "OpenOneWindow"
				" %d %d %d %d  %d %d %d %d  %d %d\n",
				proxy->proxyx, proxy->proxyy,
				proxy->proxyw, proxy->proxyh,
				other->x, other->y,
				other->w, other->h,
				proxy->group, other->group);
#endif
			if ((showOnly == PROXY_SHOWONLY_COVER ||
				!other->group ||
				proxy->group != other->group) &&
				(proxy->proxyx > other->x+other->w ||
				proxy->proxyx+proxy->proxyw < other->x ||
				proxy->proxyy > other->y+other->h ||
				proxy->proxyy+proxy->proxyh < other->y))
			{
				return;
			}
		}
	}
	if (proxy->proxy == None)
	{
		long eventMask=ButtonPressMask|ExposureMask|ButtonMotionMask;

		if (enterSelect)
		{
			eventMask|=EnterWindowMask;
		}
		attributes.override_redirect = True;
		proxy->proxy = XCreateWindow(
			dpy, rootWindow, proxy->proxyx, proxy->proxyy,
			proxy->proxyw, proxy->proxyh,border,
			DefaultDepth(dpy,screen), InputOutput, Pvisual,
			valuemask, &attributes);
		XSelectInput(dpy,proxy->proxy,eventMask);
	}
	else
	{
		XMoveWindow(dpy, proxy->proxy, proxy->proxyx, proxy->proxyy);
	}
	XMapRaised(dpy, proxy->proxy);
	DrawProxyBackground(proxy);
	proxy->flags.is_shown = 1;

	return;
}

static void OpenWindows(void)
{
	ProxyWindow *proxy;

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		OpenOneWindow(proxy);
	}

	selectProxy = NULL;
	return;
}

static void CloseOneWindow(ProxyWindow *proxy)
{
	if (proxy == NULL)
	{
		return;
	}
	if (proxy->flags.is_shown)
	{
		XUnmapWindow(dpy, proxy->proxy);
		proxy->flags.is_shown = 0;
	}

	return;
}

static void CloseWindows(void)
{
	ProxyWindow *proxy;

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		CloseOneWindow(proxy);
	}

	return;
}

static Bool SortProxiesOnce(void)
{
	Bool change=False;

	ProxyWindow *proxy;
	ProxyWindow *next;

	int x1,x2;
	int y1,y2;

	lastProxy=NULL;
	for (proxy=firstProxy; proxy != NULL && proxy->next != NULL;
		proxy=proxy->next)
	{
		x1=proxy->proxyx;
		x2=proxy->next->proxyx;
		y1=proxy->proxyy;
		y2=proxy->next->proxyy;

		/* sort x, then y, then arbitrarily on pointer */
		if ( x1>x2 || (x1==x2 && y1>y2) ||
			(x1==x2 && y1==y2 &&
			proxy->window>proxy->next->window))
		{
			change=True;
			next=proxy->next;

			if (lastProxy)
				lastProxy->next=next;
			else
				firstProxy=next;
			proxy->next=next->next;
			next->next=proxy;
		}

		lastProxy=proxy;
	}

	lastProxy=NULL;
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		proxy->prev=lastProxy;
		lastProxy=proxy;
	}

	return change;
}

static void SortProxies(void)
{
	while (SortProxiesOnce() == True)
	{
		/* nothing */
	}

	return;
}

static Bool AdjustOneWindow(ProxyWindow *proxy)
{
	Bool rc = False;
	ProxyWindow *other=proxy->next;

	for (other=proxy->next; other; other=other->next)
	{
		int dx;
		int dy;

		if(other->desk != deskNumber)
		{
			continue;
		}
		dx = abs(proxy->proxyx-other->proxyx);
		dy = abs(proxy->proxyy-other->proxyy);
		if (dx<(proxyWidth+proxySeparation) &&
				dy<proxyHeight+proxySeparation )
		{
			rc = True;
#if PROXY_GROUP_DEBUG
			fprintf(stderr,
				"AdjustOneWindow %d %d %d %d  %d %d %d %d\n",
				proxy->proxyx, proxy->proxyy,
				proxy->proxyw, proxy->proxyh,
				other->proxyx, other->proxyy,
				other->proxyw, other->proxyh);
#endif
			if (proxyWidth-dx<proxyHeight-dy)
			{
				if (proxy->proxyx<=other->proxyx)
				{
					int delta = proxy->proxyx +
						proxy->proxyw +
						proxySeparation -
						other->proxyx;
					proxy->proxyx -= delta/2;
					other->proxyx += delta - delta/2;
#if PROXY_GROUP_DEBUG
					fprintf(stderr, "left %d\n", delta);
#endif
				}
				else
				{
					int delta = other->proxyx +
						other->proxyw +
						proxySeparation -
						proxy->proxyx;
					other->proxyx -= delta/2;
					proxy->proxyx += delta - delta/2;
#if PROXY_GROUP_DEBUG
					fprintf(stderr, "right %d\n", delta);
#endif
				}
			}
			else
			{
				if (proxy->proxyy<=other->proxyy)
				{
					int delta = proxy->proxyy +
						proxy->proxyh +
						proxySeparation -
						other->proxyy;
					proxy->proxyy -= delta/2;
					other->proxyy += delta - delta/2;
#if PROXY_GROUP_DEBUG
					fprintf(stderr, "up %d\n", delta);
#endif
				}
				else
				{
					int delta = other->proxyy +
						other->proxyh +
						proxySeparation -
						proxy->proxyy;
					other->proxyy -= delta/2;
					proxy->proxyy += delta - delta/2;
#if PROXY_GROUP_DEBUG
					fprintf(stderr, "down %d\n", delta);
#endif
				}
			}
/*
			if (proxyWidth-dx<proxyHeight-dy)
			{
				if (proxy->proxyx<=other->proxyx)
				{
					other->proxyx=
						proxy->proxyx+ proxy->proxyw+
						proxySeparation;
				}
				else
				{
					proxy->proxyx=
						other->proxyx+ other->proxyw+
						proxySeparation;
				}
			}
			else
			{
				if (proxy->proxyy<=other->proxyy)
				{
					other->proxyy=
						proxy->proxyy+ proxy->proxyh+
						proxySeparation;
				}
				else
				{
					proxy->proxyy=
						other->proxyy+ other->proxyh+
						proxySeparation;
				}
			}
*/
		}
	}

	return rc;
}

static void AdjustWindows(void)
{
	Bool collision=True;

	while (collision == True)
	{
		ProxyWindow *proxy;

		collision=False;
		for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
		{
			if(proxy->desk != deskNumber)
				continue;

			if (AdjustOneWindow(proxy) == True)
			{
				collision = True;
			}
		}
	}
}

static void RecenterProxy(ProxyWindow *proxy)
{
	proxy->proxyx=proxy->x + (proxy->w-proxy->proxyw)/2;
	proxy->proxyy=proxy->y + (proxy->h-proxy->proxyh)/2;
}

static void RecalcProxyTweaks(void)
{
	ProxyWindow *proxy;
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		proxy->tweakx=proxy->proxyx -
			(proxy->x + (proxy->w-proxy->proxyw)/2);
		proxy->tweaky=proxy->proxyy -
			(proxy->y + (proxy->h-proxy->proxyh)/2);
	}
}
static void TweakProxy(ProxyWindow *proxy)
{
	proxy->proxyx += proxy->tweakx;
	proxy->proxyy += proxy->tweaky;
}

static void ReshuffleWindows(void)
{
	ProxyWindow *proxy;

#if PROXY_GROUP_DEBUG
	fprintf(stderr, "ReshuffleWindows\n");
#endif

	if (are_windows_shown)
	{
		CloseWindows();
	}
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		RecenterProxy(proxy);
	}
	AdjustWindows();
	SortProxies();
	if (are_windows_shown)
	{
		OpenWindows();
	}
	RecalcProxyTweaks();

	return;
}

static void UpdateOneWindow(ProxyWindow *proxy)
{
	if (proxy == NULL)
	{
		return;
	}
	if (proxy->flags.is_shown)
	{
		ReshuffleWindows();
	}

	return;
}

static void SendResolve(ProxyWindow *proxy)
{
	SendFvwmPipe(fd, "SendToModule FvwmProxy Resolve",
		proxy->window);
}

static void WaitToConfig(ProxyWindow *proxy)
{
	proxy->pending_config++;
	if(!waiting_to_config)
	{
		waiting_to_config=1;
		SendResolve(proxy);
	}
}

static int Provokable(ProxyWindow *proxy,proxy_provoke_t provocation)
{
	int soft=proxy->flags.is_soft;
	ProxyGroup* proxy_group=proxy->proxy_group;
	proxy_follow_t follow;
	WindowName* include;
#if PROXY_GROUP_DEBUG
	fprintf(stderr, "Provokable %p %s %x soft %d\n",
		proxy,proxy->name,provocation,soft);
#endif
	if(!proxy_group)
	{
		proxy_group=FindProxyGroupOfNeighbor(proxy);
	}
	if(!proxy_group)
	{
		return True;
	}

	follow=PROXY_FOLLOW_INHERIT;

#if PROXY_GROUP_DEBUG
	fprintf(stderr, "  group flags %x\n",proxy_group->flags);
#endif
	include = FindWindowName(proxy_group->includes,proxy->name);
	if(include)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr, "  include flags %x\n",include->flags);
#endif

		switch(provocation)
		{
			case PROXY_PROVOKE_RAISE:
				follow=soft? include->flags.soft_raise:
					include->flags.hard_raise;
			break;

			case PROXY_PROVOKE_DESK:
				follow=soft? include->flags.soft_desk:
					include->flags.hard_desk;
			break;

			case PROXY_PROVOKE_DRAG:
				follow=soft? include->flags.soft_drag:
					include->flags.hard_drag;
			break;

			case PROXY_PROVOKE_ICON:
				follow=soft? include->flags.soft_icon:
					include->flags.hard_icon;
			break;
		}
#if PROXY_GROUP_DEBUG
		fprintf(stderr, "  include follow %d\n",follow);
#endif
		if(follow!=PROXY_FOLLOW_INHERIT)
		{
			return (follow!=PROXY_FOLLOW_OFF);
		}
	}

	follow=PROXY_FOLLOW_INHERIT;
	switch(provocation)
	{
		case PROXY_PROVOKE_RAISE:
			follow=soft? proxy_group->flags.soft_raise:
				proxy_group->flags.hard_raise;
		break;

		case PROXY_PROVOKE_DESK:
			follow=soft? proxy_group->flags.soft_desk:
				proxy_group->flags.hard_desk;
		break;

		case PROXY_PROVOKE_DRAG:
			follow=soft? proxy_group->flags.soft_drag:
				proxy_group->flags.hard_drag;
		break;

		case PROXY_PROVOKE_ICON:
			follow=soft? proxy_group->flags.soft_icon:
				proxy_group->flags.hard_icon;
		break;
	}
#if PROXY_GROUP_DEBUG
	fprintf(stderr, "  group follow %d\n",follow);
#endif
	return (follow!=PROXY_FOLLOW_OFF);
}

static int Provoked(ProxyWindow *instigator,ProxyWindow *proxy,
	proxy_provoke_t provocation)
{
	return (Provokable(instigator,provocation) &&
		Provokable(proxy,provocation));
}

static void IsolateGroup(ProxyWindow *instigator,int isolate)
{
	ProxyWindow *proxy;

#if PROXY_GROUP_DEBUG
	fprintf(stderr, "IsolateGroup %p %d\n",
		instigator,isolate);
#endif

	if(!instigator->group)
	{
		return;
	}

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(proxy==instigator)
		{
			continue;
		}
		if (proxy->desk == deskNumber &&
			proxy->group==instigator->group)
		{
			proxy->flags.is_isolated=isolate;

			DrawProxyBackground(proxy);
			DrawProxy(proxy);
		}
	}
}

static void IconifyGroup(ProxyWindow *instigator,int iconify)
{
	ProxyWindow *proxy;

#if PROXY_GROUP_DEBUG
	fprintf(stderr, "IconifyGroup %p %d\n",
		instigator,iconify);
#endif

	if(!instigator->group)
	{
		return;
	}

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(proxy==instigator)
		{
			continue;
		}
		if(!Provoked(instigator,proxy,PROXY_PROVOKE_ICON))
		{
			continue;
		}
		if (proxy->desk == deskNumber &&
			proxy->group==instigator->group)
		{
			sprintf(commandBuffer,"Iconify %s",
				iconify? "On": "Off");
			send_command_to_fvwm(commandBuffer,proxy->window);
		}
	}
}

void RefineStack(int desired)
{
	ProxyWindow *proxy;
	ProxyWindow *best = NULL;
	int index = 0;
#if PROXY_GROUP_DEBUG
	fprintf(stderr, "RefineStack %d\n", desired);
#endif
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		proxy->stack_tmp = -1;
	}
	while(True)
	{
		best = NULL;
		for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
		{
			if(proxy->stack_tmp < 0 && (best == NULL ||
				(desired? proxy->stack_desired: proxy->stack) <
				(desired? best->stack_desired: best->stack)))
			{
				best = proxy;
			}
		}
		if(best == NULL)
		{
			break;
		}
#if PROXY_GROUP_DEBUG
		fprintf(stderr, "%p %d %d -> %d %s\n", best,
			best->stack, best->stack_desired,
			index, best->name);
#endif
		best->stack_tmp = index++;
	}
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(desired)
		{
			proxy->stack_desired = proxy->stack_tmp;
		}
		else
		{
			proxy->stack = proxy->stack_tmp;
		}
	}
}

/* doesn't work: non-fatal X error in X_ConfigureWindow */
void RestackAtomic(int raise)
{
	ProxyWindow *proxy;
	int count = 0;
	Window *windows;
	RefineStack(False);

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		count++;
	}
	windows = xmalloc(count * sizeof(Window));
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		fprintf(stderr, "RestackAtomic %d %s\n",
			proxy->stack, proxy->name);
		windows[proxy->stack] = proxy->window;
	}
	XRestackWindows(dpy, windows, count);
	free (windows);
}

void RestackIncremental(int raise)
{
	int changes = 0;
	int changed = 1;

	RefineStack(False);
	RefineStack(True);

	while(changed)
	{
		ProxyWindow *proxy;
		ProxyWindow *best = NULL;
		int next_delta = 0;
		changed = 0;

#if PROXY_GROUP_DEBUG
		for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
		{
			fprintf(stderr, "%2d ", proxy->stack);
		}
		fprintf(stderr, "\n");
		for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
		{
			fprintf(stderr, "%2d ", proxy->stack_desired);
		}
		fprintf(stderr, "\n");
#endif
		for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
		{
			int delta = raise?
				proxy->stack - proxy->stack_desired:
				proxy->stack_desired - proxy->stack;
			if(delta > 0 &&
				(best == NULL || delta > next_delta))
			{
				best = proxy;
				next_delta = delta;
			}
		}
		if(best)
		{
#if PROXY_GROUP_DEBUG
			fprintf(stderr,
				"RestackIncremental Raise %p %s %d -> %d\n",
				best, best->name,
				best->stack, best->stack_desired);
#endif
			if(raise)
			{
				best->raised=1;
				XRaiseWindow(dpy, best->window);
				best->stack = INT_MIN;
			}
			else
			{
				best->raised= -1;
				XLowerWindow(dpy, best->window);
				best->stack = INT_MAX;
			}
			RefineStack(False);
			changed = 1;
		}
		if(++changes > 32)
		{
#if PROXY_GROUP_DEBUG
			fprintf(stderr,
				"RestackIncremental excessive changes (%d)\n",
				changes);
#endif
			break;
		}
#if PROXY_GROUP_DEBUG
		fprintf(stderr, "*****************************************\n");
		fprintf(stderr, "RestackIncremental(%d) %d changes\n",
			raise, changes);
#endif
	}
}

void Restack(int raise)
{
	RestackIncremental(raise);
}

void ReadStack(unsigned long *body, unsigned long length)
{
	Window window;
	ProxyWindow *proxy;
	int i;
	ProxyWindow *instigator=NULL;
	int index;

#if PROXY_GROUP_DEBUG
	fprintf(stderr, "ReadStack %p %d-%d\n",
		body, (int)length, FvwmPacketHeaderSize);
#endif

	for (i = 0; i < (length - FvwmPacketHeaderSize); i += 3)
	{
		instigator=FindProxy(body[0]);
		if(instigator)
		{
			break;
		}
	}
	if(!instigator)
	{
		return;
	}
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(proxy->stack>instigator->stack)
		{
			proxy->stack+=10000;
		}
	}

	index=instigator->stack;
	for (i = 0; i < (length - FvwmPacketHeaderSize); i += 3)
	{
		window = body[i];
#if PROXY_GROUP_DEBUG
		fprintf(stderr, "  stack %d %d\n", i, (int)window);
#endif
		proxy = FindProxy(window);
		if(proxy)
		{
#if PROXY_GROUP_DEBUG
			fprintf(stderr, " %p %s",proxy,proxy->name);
#endif
			proxy->stack = index++;
		}
#if PROXY_GROUP_DEBUG
		fprintf(stderr, "\n");
#endif
	}
	RefineStack(False);
}

static void RaiseLowerGroup(Window w,int raise)
{
	ProxyWindow *instigator;
	ProxyWindow *proxy;

	instigator = FindProxy(w);
#if PROXY_GROUP_DEBUG
	fprintf(stderr, "RaiseLowerGroup %d %s\n", raise,
			instigator? instigator->name: "<none>");
#endif
	if(instigator==NULL)
	{
		return;
	}
	instigator->stack = raise? INT_MIN: INT_MAX;
	RefineStack(False);
	if(instigator->flags.is_isolated)
	{
		return;
	}
	if(!Provokable(instigator,PROXY_PROVOKE_RAISE))
	{
		return;
	}

#if PROXY_GROUP_DEBUG
	fprintf(stderr, "RaiseLowerGroup %p %d %d\n",
		instigator,raise,instigator->raised);
#endif

	if(abs(instigator->raised)>10)
	{
		exit(1);
	}

	if(instigator->raised == (raise? 1: -1))
	{
		return;
	}

	instigator->raised=0;

	if(instigator->flags.is_iconified || !instigator->group)
	{
		return;
	}

	instigator->stack = raise? INT_MIN: INT_MAX;
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		proxy->stack_desired = proxy->stack;
	}
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(proxy==instigator)
		{
			continue;
		}
		if(!Provokable(proxy,PROXY_PROVOKE_RAISE))
		{
			continue;
		}
		if (proxy->desk == deskNumber &&
			proxy->group==instigator->group)
		{
			if(raise)
			{
#if PROXY_GROUP_DEBUG
				fprintf(stderr, "Raise %p\n",proxy);
#endif

				proxy->stack_desired-=10000;
			}
			else
			{
#if PROXY_GROUP_DEBUG
				fprintf(stderr, "Lower %p\n",proxy);
#endif

				proxy->stack_desired+=10000;
			}
		}
	}
	Restack(raise);
}

/*
static void RaiseLowerGroupOld(Window w,int raise)
{
	ProxyWindow *instigator;
	ProxyWindow *proxy;

	instigator = FindProxy(w);
	fprintf(stderr, "RaiseLowerGroup %d %s\n", raise,
			instigator? instigator->name: "<none>");
	if(instigator)
	{
		instigator->stack = raise? INT_MIN: INT_MAX;
		RefineStack(False);
	}
	if(instigator==NULL || instigator->flags.is_isolated)
	{
		return;
	}


#if PROXY_GROUP_DEBUG
	fprintf(stderr, "RaiseLowerGroup %p %d %d\n",
		instigator,raise,instigator->raised);
#endif

	if(abs(instigator->raised)>10)
	{
		exit(1);
	}

	if(instigator->raised == (raise? 1: -1))
	{
		return;
	}

	instigator->raised=0;

	if(instigator->flags.is_iconified || !instigator->group)
	{
		return;
	}

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(proxy==instigator || proxy->raised == (raise? 1: -1))
		{
			continue;
		}
		if (proxy->desk == deskNumber &&
			proxy->group==instigator->group)
		{
			if(raise)
			{
#if PROXY_GROUP_DEBUG
				fprintf(stderr, "Raise %p\n",proxy);
#endif

				proxy->raised=1;
				XRaiseWindow(dpy, proxy->window);
			}
			else
			{
#if PROXY_GROUP_DEBUG
				fprintf(stderr, "Lower %p\n",proxy);
#endif

				proxy->raised= -1;
				XLowerWindow(dpy, proxy->window);
			}
		}
	}
	if(raise)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr, "ReRaise %p\n",instigator);
#endif

		instigator->raised=1;
		XRaiseWindow(dpy, instigator->window);
	}
	else
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr, "ReLower %p\n",instigator);
#endif

		instigator->raised= -1;
		XLowerWindow(dpy, instigator->window);
	}
}
*/

static void ClearRaised(void)
{
	ProxyWindow *proxy;

#if PROXY_GROUP_DEBUG
	fprintf(stderr, "ClearRaised\n");
#endif

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		proxy->raised=0;
	}
}

static void ShiftWindows(ProxyWindow *instigator,int dx,int dy)
{
	ProxyWindow *proxy;

	if((are_windows_shown && !instigator->flags.is_isolated) ||
		!instigator->group || instigator->flags.is_soft)
	{
		return;
	}

#if PROXY_GROUP_DEBUG
	fprintf(stderr, "ShiftWindows %d %d %d\n",
		instigator->group,dx,dy);
#endif

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(proxy==instigator)
		{
			continue;
		}
		if(!instigator->flags.is_isolated &&
			!Provoked(instigator,proxy,PROXY_PROVOKE_DRAG))
		{
			continue;
		}
		if (proxy->desk == deskNumber &&
			proxy->group==instigator->group)
		{
			proxy->x+=dx;
			proxy->y+=dy;
/*
			XMoveWindow(dpy, proxy->window,
				proxy->x+proxy->border_width,
				proxy->y+proxy->title_height+
				proxy->border_width);
*/
#if PROXY_GROUP_DEBUG
			fprintf(stderr, "shift %d %d (%d %d)\n",
				proxy->x,proxy->y, dx,dy);
#endif
			WaitToConfig(proxy);
		}
	}
}

static void CatchWindows(ProxyWindow *instigator,int vertical,int from,int to,
	int direction)
{
	ProxyWindow *proxy;

	if((are_windows_shown && !instigator->flags.is_isolated) ||
		!instigator->group || instigator->flags.is_soft)
	{
		return;
	}

#if PROXY_GROUP_DEBUG
	fprintf(stderr, "CatchWindows %d %d %d %d\n",
		instigator->group,vertical,from,to);
#endif

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(proxy==instigator)
		{
			continue;
		}
		if(!Provoked(instigator,proxy,PROXY_PROVOKE_DRAG))
		{
			continue;
		}
		if (proxy->desk == deskNumber &&
			proxy->group==instigator->group &&
			!proxy->flags.is_soft)
		{
			int changed=0;
			int newx=proxy->x;
			int newy=proxy->y;
			int neww=proxy->goal_width;
			int newh=proxy->goal_height;
			const int incx=proxy->incx;
			const int incy=proxy->incy;
/*			int threshold=(vertical? incy: incx)-1;
*/
			int threshold=0;
			int least=from;
			int most=from;

			if(direction<0)
			{
				least-=threshold;
			}
			else
			{
				most+=threshold;
			}

#if PROXY_GROUP_DEBUG
			fprintf(stderr,
				"check %p %d %d size %d %d"
				" goal %d %d inc %d %d\n",
				proxy,proxy->x,proxy->y,proxy->w,proxy->h,
				proxy->goal_width,proxy->goal_height,
				incx,incy);
#endif

			if(vertical)
			{
				if(newy>=least && newy<=most)
				{
					newy=to;
					newh-=to-from;
					changed=1;
				}
				else if(newy+newh>=least &&
					newy+newh<=most)
				{
					newh=to-newy;
					changed=1;
				}
			}
			else
			{
				if(newx>=least && newx<=most)
				{
					newx=to;
					neww-=to-from;
					changed=1;
				}
				else if(newx+neww>=least &&
					newx+neww<=most)
				{
					neww=to-newx;
					changed=1;
				}
			}
			if(changed)
			{
#if PROXY_GROUP_DEBUG
				fprintf(stderr, "change %d %d %d %d\n",
					newx,newy,neww,newh);
#endif
				if(newx!=proxy->x || newy!=proxy->y)
				{
#if PROXY_GROUP_DEBUG
					fprintf(stderr, "move\n");
#endif
/*
					XMoveWindow(dpy, proxy->window,
						newx+bw,
						newy+th+bw);
*/
					WaitToConfig(proxy);

					/*	in case more motion of instigator
						precedes this window's config */
					proxy->x=newx;
					proxy->y=newy;
				}
				if(neww<proxy->w ||
					neww>=proxy->w+incx ||
					newh<proxy->h ||
					newh>=proxy->h+incy)
				{
#if PROXY_GROUP_DEBUG
					fprintf(stderr, "resize\n");
#endif
/*
					XResizeWindow(dpy, proxy->window,
						neww-2*bw,
						newh-2*bw-th);
*/
					WaitToConfig(proxy);

					/*	in case more motion of instigator
						precedes this window's config */
					if(neww<proxy->w)
					{
						proxy->w-=((proxy->w-neww-1)/
							incx+1)*incx;
					}
					else if(neww>=proxy->w+incx)
					{
						proxy->w+=(neww-proxy->w)/incx
							*incx;
					}
					if(newh<proxy->h)
					{
						proxy->h-=((proxy->h-newh-1)/
							incy+1)*incy;
					}
					else if(newh>=proxy->h+incy)
					{
						proxy->h+=(newh-proxy->h)/incy
							*incy;
					}
				}
				proxy->goal_width=neww;
				proxy->goal_height=newh;

			}
		}
	}
}

static void MoveProxiedWindow(ProxyWindow* proxy,int x,int y,int w,int h)
{
#if 1
	sprintf(commandBuffer,"ResizeMove frame %dp %dp +%dp +%dp",
		w,
		h,
		x,
		y);
	send_command_to_fvwm(commandBuffer,proxy->window);
#else
	const int bw=proxy->border_width;
	const int th=proxy->title_height;

	XMoveResizeWindow(dpy, proxy->window,
		x+bw,
		y+th+bw,
		w-2*bw,
		h-2*bw-th);
#endif
}

static void ResolvePendingWindows(void)
{
	ProxyWindow *proxy;

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(proxy->pending_config)
		{
			MoveProxiedWindow(proxy,proxy->x,proxy->y,
				proxy->goal_width,proxy->goal_height);
		}
	}
}

int FindUniqueGroup(int desk)
{
	ProxyWindow *proxy;

	/* find unique group */
	int group=1;
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if (proxy->desk == desk && proxy->group==group)
		{
			group++;
			proxy=firstProxy;
			continue;
		}
	}
#if PROXY_GROUP_DEBUG
	fprintf(stderr,"FindUniqueGroup desk %d group %d\n",desk,group);
#endif
	return group;
}

static void MoveGroupToDesk(ProxyWindow *instigator,int desk)
{
	ProxyWindow *proxy;
	int old_desk;
	int old_group;
	int group;

#if PROXY_GROUP_DEBUG
	fprintf(stderr, "MoveGroupToDesk %p %d\n",
		instigator,desk);
#endif

	if(!instigator->group)
	{
		return;
	}

	old_desk=instigator->desk;
	old_group=instigator->group;
	group=FindUniqueGroup(desk);

	sprintf(commandBuffer, "MoveToDesk 0 %d", desk);
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(!Provoked(instigator,proxy,PROXY_PROVOKE_DESK))
		{
			continue;
		}
		if (proxy->desk == old_desk &&
			proxy->group==old_group)
		{
			proxy->desk=desk;
			proxy->group=group;

			send_command_to_fvwm( commandBuffer, proxy->window);

			DrawProxyBackground(proxy);
			DrawProxy(proxy);
		}
	}
}


static ProxyWindow* FindNeighborInGroup(ProxyWindow* instigator,
	int not_iconified)
{
	ProxyWindow *proxy;

	if(!instigator)
	{
		return NULL;
	}

#if PROXY_GROUP_DEBUG
	fprintf(stderr,"FindNeighborInGroup %p %p desk %d group %d\n",
		instigator,instigator->proxy_group,instigator->desk,
		instigator->group);
#endif

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr," vs %p %p desk %d group %d\n",
			proxy,proxy->proxy_group,proxy->desk,proxy->group);
#endif
		if (proxy!= instigator && proxy->desk == instigator->desk &&
			proxy->group &&
			proxy->group==instigator->group &&
			(!not_iconified || !proxy->flags.is_iconified))
		{
			return proxy;
		}
	}

	return NULL;
}

static ProxyWindow* FindNeighborForProxy(ProxyWindow* instigator)
{
	ProxyWindow *proxy;
	ProxyWindow *neighbor=NULL;
	int group;

	if(!instigator)
	{
		return NULL;
	}

#if PROXY_GROUP_DEBUG
	fprintf(stderr,"FindNeighborForProxy %p %p desk %d\n",
		instigator,instigator->proxy_group,instigator->desk);
#endif

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr," vs %p %p desk %d group %d\n",
			proxy,proxy->proxy_group,proxy->desk,proxy->group);
#endif
		if (proxy!= instigator && proxy->desk == instigator->desk &&
			proxy->proxy_group==instigator->proxy_group)
		{
			if(proxy->proxy_group->flags.isolated &&
				!instigator->flags.is_isolated)
			{
				SendFvwmPipe(fd,
					"SendToModule FvwmProxy IsolateToggle",
					proxy->window);
			}
			if(proxy->group)
			{
				return proxy;
			}
			neighbor=proxy;
		}
	}

	if(!neighbor)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr,"not found\n");
#endif
		return NULL;
	}

	group=FindUniqueGroup(instigator->desk);

	/* assign group to all proxies with given pid */
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if (proxy->desk == instigator->desk &&
			proxy->proxy_group==instigator->proxy_group)
		{
#if PROXY_GROUP_DEBUG
			fprintf(stderr,"unique group %d reassign %s\n",
				group,proxy->name);
#endif
			proxy->group=group;
		}
	}

#if PROXY_GROUP_DEBUG
	fprintf(stderr,"group %d\n",group);
#endif
	return neighbor;
}

static ProxyWindow* FindNeighborForProcess(ProxyWindow* proxy,
	int desk,int pid,int ppid)
{
	ProxyWindow *other;
	ProxyWindow *neighbor=NULL;
	int group;
	int auto_include=0;

#if PROXY_GROUP_DEBUG
		fprintf(stderr,"FindNeighborForProcess %p desk %d pid %d %d\n",
			proxy, desk, ppid, pid);
#endif

	if(!pid)
	{
		return 0;
	}

	/* find existing group for pid */
	for (other=firstProxy; other != NULL; other=other->next)
	{
		if (other->desk == desk && other->proxy_group &&
			(other->pid==pid || (ppid && other->pid==ppid) ||
			other->ppid==pid) &&
			(MatchWindowName(other->proxy_group->includes,
				proxy->name) ||
			(auto_include=other->proxy_group->flags.auto_include)))
		{
			proxy->flags.is_soft = (auto_include &&
				other->proxy_group->flags.auto_soft);
			if(other->proxy_group->flags.isolated &&
				!proxy->flags.is_isolated)
			{
				SendFvwmPipe(fd,
					"SendToModule FvwmProxy IsolateToggle",
					proxy->window);
			}
#if PROXY_GROUP_DEBUG
			fprintf(stderr,
				"pid %d %d found %d %d group %d soft %d\n",
				pid,ppid,other->pid,other->ppid,
				other->group,proxy->flags.is_soft);
#endif
			if(other->group)
			{
				return other;
			}
			neighbor=other;
		}
	}
	if(!neighbor)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr,"FindNeighborForProcess pid %d %d not found\n",
			pid,ppid);
#endif
		return NULL;
	}

	group=FindUniqueGroup(desk);

	/* assign pid to all proxies with given pid */
	for (other=firstProxy; other != NULL; other=other->next)
	{
		if (other->desk == desk && other->proxy_group &&
			(other->pid==pid || (ppid && other->pid==ppid) ||
			other->ppid==pid))
		{
#if PROXY_GROUP_DEBUG
			fprintf(stderr,"unique group %d reassign %s\n",
				group,other->name);
#endif
			other->group=group;
		}
	}

#if PROXY_GROUP_DEBUG
	fprintf(stderr,"FindNeighborForProcess pid %d new %d\n",pid,group);
#endif
	return neighbor;
}

static ProxyWindow* FindNeighborForLeader(ProxyWindow* proxy,
	int desk,int leader)
{
	ProxyWindow *other;
	ProxyWindow *neighbor=NULL;
	int group;
	int auto_include=0;

#if PROXY_GROUP_DEBUG
		fprintf(stderr,"FindNeighborForLeader %p desk %d leader %d \n",
			proxy, desk, leader);
#endif

	if(!leader)
	{
		return 0;
	}

	/* find existing group for leader */
	for (other=firstProxy; other != NULL; other=other->next)
	{
		if (other->desk == desk && other->proxy_group &&
			other->leader==leader &&
			(MatchWindowName(other->proxy_group->includes,
				proxy->name) ||
			(auto_include=other->proxy_group->flags.auto_include)))
		{
			proxy->flags.is_soft=auto_include &&
				other->proxy_group->flags.auto_soft;
			if(other->proxy_group->flags.isolated &&
				!proxy->flags.is_isolated)
			{
				SendFvwmPipe(fd,
					"SendToModule FvwmProxy IsolateToggle",
					proxy->window);
			}
#if PROXY_GROUP_DEBUG
			fprintf(stderr,"leader %d found %d group %d soft %d\n",
				leader,other->leader,
				other->group,proxy->flags.is_soft);
#endif
			if(other->group)
			{
				return other;
			}
			neighbor=other;
		}
	}
	if(!neighbor)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr,"FindNeighborForLeader leader %d not found\n",
			leader);
#endif
		return NULL;
	}

	group=FindUniqueGroup(desk);

	/* assign leader to all proxies with given leader */
	for (other=firstProxy; other != NULL; other=other->next)
	{
		if (other->desk == desk && other->leader==leader &&
			other->proxy_group)
		{
#if PROXY_GROUP_DEBUG
			fprintf(stderr,"unique group %d reassign %s\n",
				group,other->name);
#endif
			other->group=group;
		}
	}

#if PROXY_GROUP_DEBUG
	fprintf(stderr,"FindNeighborForLeader leader %x new %d\n",
		leader,group);
#endif
	return neighbor;
}

static ProxyWindow* FindNeighborForApplication(ProxyWindow* proxy)
{
	ProxyWindow* neighbor=FindNeighborForLeader(proxy,proxy->desk,
		proxy->leader);
	if(!neighbor)
	{
		neighbor=FindNeighborForProcess(proxy,proxy->desk,
			proxy->pid,proxy->ppid);
	}
	return neighbor;
}

static int ProxyGroupCheckSoft(ProxyGroup* proxy_group,
	ProxyWindow* proxy)
{
	WindowName* include=FindWindowName(proxy_group->includes,
		proxy->name);
	return include? include->flags.is_soft: 0;
}

static int ProxyGroupCheckWeak(ProxyGroup* proxy_group,
	ProxyWindow* proxy)
{
	WindowName* include=FindWindowName(proxy_group->includes,
		proxy->name);
	return include? include->flags.is_weak: 0;
}

static void UpdateProxyGroup(ProxyWindow* proxy)
{
	ProxyWindow* neighbor;
#if PROXY_GROUP_DEBUG
	fprintf(stderr,"UpdateProxyGroup %s\n",proxy->name);
#endif

	if(proxy->proxy_group)
	{
		return;
	}
	if(!proxy->proxy_group)
	{
		neighbor=FindNeighborForApplication(proxy);
/*
		proxy->group=0;
*/
		proxy->proxy_group=NULL;
		if(neighbor && neighbor->proxy_group)
		{
			proxy->group=neighbor->group;
			proxy->proxy_group=neighbor->proxy_group;
		}
	}
	if(!proxy->proxy_group)
	{
		proxy->proxy_group=
			FindProxyGroupWithWindowName(proxy->name);
		if(proxy->proxy_group &&
			ProxyGroupCheckWeak(proxy->proxy_group,proxy))
		{
			proxy->proxy_group=NULL;
		}
	}
	if(proxy->proxy_group &&
		MatchWindowName(proxy->proxy_group->excludes,proxy->name))
	{
		proxy->proxy_group=NULL;
		proxy->group=0;
	}
	if(proxy->proxy_group && !proxy->group &&
		proxy->proxy_group->flags.ignore_ids)
	{
		neighbor=FindNeighborForProxy(proxy);
		if(neighbor)
		{
			proxy->group=neighbor->group;
		}
	}

	if(proxy->proxy_group)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr," ProxyGroup %s\n",proxy->proxy_group->name);
#endif

		proxy->flags.is_soft=proxy->flags.is_soft ||
			ProxyGroupCheckSoft(proxy->proxy_group,proxy);
	}
}

static void UpdateProxyGroupForAll(void)
{
	ProxyWindow *proxy;

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		UpdateProxyGroup(proxy);
	}
}

static void AdhereGroup(ProxyWindow *instigator)
{
	ProxyWindow *proxy;

#if PROXY_GROUP_DEBUG
	fprintf(stderr, "AdhereGroup %p\n", instigator);
#endif

	if(!instigator)
	{
		return;
	}

	if(!instigator->group || instigator->flags.is_soft)
	{
		return;
	}

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(proxy==instigator)
		{
			continue;
		}
		if (proxy->desk == instigator->desk &&
			proxy->group==instigator->group)
		{
			if(!proxy->flags.is_soft &&
				(proxy->x != instigator->x ||
				proxy->y != instigator->y ||
				proxy->goal_width != instigator->goal_width ||
				proxy->goal_height != instigator->goal_height))
			{
				proxy->x=instigator->x;
				proxy->y=instigator->y;
				proxy->goal_width=instigator->goal_width;
				proxy->goal_height=instigator->goal_height;

				MoveProxiedWindow(proxy,proxy->x,proxy->y,
					proxy->goal_width,proxy->goal_height);
			}
		}
	}
}

static void SetGroup(ProxyWindow* proxy, int group)
{
	ProxyWindow *neighbor;

	proxy->group=group;
	if(group)
	{
		neighbor=FindNeighborInGroup(proxy,0);
		if(neighbor)
		{
			proxy->flags.is_isolated = neighbor->flags.is_isolated;
			if(proxy->flags.is_isolated)
			{
				AdhereGroup(neighbor);
			}
		}
		if(proxy->flags.is_isolated && !proxy->flags.is_iconified)
		{
			IconifyGroup(proxy,1);
		}
	}
	else
	{
		proxy->flags.is_soft=0;
		proxy->flags.is_isolated=0;
	}
}

void DumpStampQueue(void)
{
#if PROXY_GROUP_DEBUG
	int m;

	fprintf(stderr,"DumpStampQueue stamp %d min %d max %d\n",
		stamp,stampMin,stampMax);
	for(m=0;m<stampLimit;m++)
	{
		fprintf(stderr,"stampQueue[%d] %x %d %d %d %d\n",m,
			(int)(stampQueue[m].window),
			stampQueue[m].x,
			stampQueue[m].y,
			stampQueue[m].w,
			stampQueue[m].h);
	}
#endif
}

void StoreStamp(Window window,int x,int y,int w,int h)
{
#if PROXY_GROUP_DEBUG
	fprintf(stderr,"StoreStamp %x %d %d %d %d\n",(int)window,x,y,w,h);
#endif

	stamp=(stamp+1)%stampLimit;
	stampMax=stamp;
	if(stampMin==stampMax)
	{
		stampMin=(stampMax+1)%stampLimit;
	}

	stampQueue[stamp].window=window;
	stampQueue[stamp].x=x;
	stampQueue[stamp].y=y;
	stampQueue[stamp].w=w;
	stampQueue[stamp].h=h;

	DumpStampQueue();
}

void UndoStamp(void)
{
	ProxyWindow *proxy;

	if(stamp==stampMin)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr,"UndoStamp empty, stamp %d min %d max %d\n",
			stamp,stampMin,stampMax);
#endif
		return;
	}
	proxy = FindProxy(stampQueue[stamp].window);
	if(!proxy)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr,"UndoStamp no proxy for window\n");
#endif
		return;
	}

	pending_do++;
	MoveProxiedWindow(proxy,
		stampQueue[stamp].x,
		stampQueue[stamp].y,
		stampQueue[stamp].w,
		stampQueue[stamp].h);

	stampQueue[stamp].window=proxy->window;
	stampQueue[stamp].x=proxy->x;
	stampQueue[stamp].y=proxy->y;
	stampQueue[stamp].w=proxy->w;
	stampQueue[stamp].h=proxy->h;

	stamp=(stamp-1+stampLimit)%stampLimit;

	DumpStampQueue();

	SendResolve(proxy);
}

void RedoStamp(void)
{
	ProxyWindow *proxy;

	if(stamp==stampMax)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr,"RedoStamp empty, stamp %d min %d max %d\n",
			stamp,stampMin,stampMax);
#endif
		return;
	}

	stamp=(stamp+1)%stampLimit;

	proxy = FindProxy(stampQueue[stamp].window);
	if(!proxy)
	{
#if PROXY_GROUP_DEBUG
		fprintf(stderr,"RedoStamp no proxy for window\n");
#endif
		return;
	}

	pending_do++;
	MoveProxiedWindow(proxy,
		stampQueue[stamp].x,
		stampQueue[stamp].y,
		stampQueue[stamp].w,
		stampQueue[stamp].h);

	stampQueue[stamp].window=proxy->window;
	stampQueue[stamp].x=proxy->x;
	stampQueue[stamp].y=proxy->y;
	stampQueue[stamp].w=proxy->w;
	stampQueue[stamp].h=proxy->h;

	DumpStampQueue();

	SendResolve(proxy);
}

static void ConfigureWindow(FvwmPacket *packet)
{
	unsigned long* body = packet->body;

	struct ConfigWinPacket *cfgpacket=(void *)body;
	int wx=cfgpacket->frame_x;
	int wy=cfgpacket->frame_y;
	int desk=cfgpacket->desk;
	int wsx=cfgpacket->frame_width;
	int wsy=cfgpacket->frame_height;
	int border_width=cfgpacket->border_width;
	int title_height=cfgpacket->title_height;
	int incx=cfgpacket->hints_width_inc;
	int incy=cfgpacket->hints_height_inc;
	Window target=cfgpacket->w;
	int leader;
	int pid;
	int ppid;
	ProxyWindow *proxy;
	int is_new_window = 0;

	if (DO_SKIP_WINDOW_LIST(cfgpacket))
	{
		return;
	}
#if PROXY_GROUP_DEBUG
	fprintf(stderr,"\n");
#endif
	leader=GetLeader(target);
	pid=GetProcessId(target);
	ppid=GetParentProcessId(pid);
	proxy = FindProxy(target);
	if (proxy == NULL)
	{
		is_new_window = 1;
		proxy=new_ProxyWindow();
		proxy->next = firstProxy;
		firstProxy = proxy;
		proxy->window=target;

		/*	unreliable on existing windows
			on 2.5.10, reporting false just after M_ICONIFY */
		proxy->flags.is_iconified = !!IS_ICONIFIED(cfgpacket);
	}

#if PROXY_GROUP_DEBUG
	fprintf(stderr,
		"Config %p %x ld %x pid=%d %d\n"
		" pos %d %d sz %d %d"
		" was %d %d sz %d %d goal %d %d"
		" bdr %d %d pend %d %d\n",
		proxy,(int)target,leader,pid,ppid,
		wx,wy,wsx,wsy,
		proxy->x,proxy->y,
		proxy->w,proxy->h,
		proxy->goal_width,proxy->goal_height,
		border_width,title_height,
		proxy->pending_config,
		pending_do);
#endif

	if(proxy->pending_config)
	{
		proxy->pending_config=0;
	}
	else
	{
		if(!pending_do && !waiting_to_stamp &&
			(is_new_window || proxy->x!=wx || proxy->y!=wy ||
			proxy->w!=wsx || proxy->h!=wsy))
		{
			waiting_to_stamp=1;
			sprintf(commandBuffer,
				"SendToModule FvwmProxy Stamp %d %d %d %d",
				proxy->x,proxy->y,proxy->w,proxy->h);
			SendFvwmPipe(fd, commandBuffer, proxy->window);
		}

		if (is_new_window)
		{
			proxy->goal_width=wsx;
			proxy->goal_height=wsy;
		}
		else
		{
			int shifted=0;
			int wasx=proxy->x;
			int wasy=proxy->y;
			int wasw=proxy->w;
			int wash=proxy->h;
			int wasgw=proxy->goal_width;
			int wasgh=proxy->goal_height;
			if(proxy->x!=wx && proxy->w==wsx)
			{
				ShiftWindows(proxy,wx-proxy->x,wy-proxy->y);
				shifted=1;
			}
			else
			{
				if(wasx!=wx)
				{
					CatchWindows(proxy,0,proxy->x,wx,-1);
					proxy->goal_width=wsx;
				}
				if(wasx+wasw!=wx+wsx)
				{
					CatchWindows(proxy,0,
						wasx+wasgw,
						wx+wsx,1);
					proxy->goal_width=wsx;
				}
			}

			if(proxy->y!=wy && proxy->h==wsy)
			{
				if(!shifted)
				{
					ShiftWindows(proxy,0,wy-proxy->y);
				}
			}
			else
			{
				if(wasy!=wy)
				{
					CatchWindows(proxy,1,proxy->y,wy,-1);
					proxy->goal_height=wsy;
				}
				if(wasy+wash!=wy+wsy)
				{
					CatchWindows(proxy,1,
						wasy+wasgh,
						wy+wsy,1);
					proxy->goal_height=wsy;
				}
			}
/*
			if(wx!=proxy->x || wy!=proxy->y ||
				wsx!=proxy->w || wsy!=proxy->h)
			{
				StoreStamp(target,proxy->x,proxy->y,
					proxy->w,proxy->h);
			}
*/
		}
	}

	if(!is_new_window && proxy->desk!=desk)
	{
		MoveGroupToDesk(proxy,desk);
	}

	proxy->leader=leader;
	proxy->pid=pid;
	proxy->ppid=ppid;
	proxy->x=wx;
	proxy->y=wy;
	proxy->desk=desk;
	proxy->w=wsx;
	proxy->h=wsy;
	proxy->proxyw=proxyWidth;
	proxy->proxyh=proxyHeight;
	proxy->border_width=border_width;
	proxy->title_height=title_height;
	proxy->incx=incx;
	proxy->incy=incy;

	RecenterProxy(proxy);
	if (!is_new_window)
	{
		TweakProxy(proxy);
	}

	if (are_windows_shown)
	{
		if (is_new_window)
		{
			ReshuffleWindows();
		}
		else
		{
			CloseOneWindow(proxy);
			OpenOneWindow(proxy);
		}
	}

	if(proxy->flags.is_isolated)
	{
		AdhereGroup(proxy);
	}

	return;
}

static void IconifyWindow(ProxyWindow *proxy, int is_iconified)
{
	if (proxy == NULL)
	{
		return;
	}
	is_iconified = !!is_iconified;
	if(proxy->flags.is_iconified != is_iconified)
	{
		proxy->flags.is_iconified = is_iconified;
		if (!proxyIconified && is_iconified)
		{
			if (proxy->flags.is_shown)
			{
				CloseOneWindow(proxy);
			}
		}
		else
		{
			if (are_windows_shown)
			{
/*				ReshuffleWindows();
*/
				OpenOneWindow(proxy);
			}
		}

		DrawProxyBackground(proxy);

		if(proxy->flags.is_isolated)
		{
			if(!is_iconified)
			{
				IconifyGroup(proxy, 1);
			}
		}
		else
		{
			IconifyGroup(proxy,is_iconified);
		}
	}

	return;
}

static void IsolateCheck(ProxyWindow *instigator,int force_other)
{
	if(instigator->flags.is_isolated)
	{
		if(!force_other && !instigator->flags.is_iconified)
		{
			IconifyGroup(instigator,1);
			AdhereGroup(instigator);
		}
		else
		{
			ProxyWindow *neighbor=
				FindNeighborInGroup(instigator,1);
			if(neighbor)
			{
				IconifyGroup(neighbor,1);
			}
			else
			{
				neighbor=FindNeighborInGroup(instigator,0);
				if(neighbor && force_other)
				{
					sprintf(commandBuffer,"Iconify Off");
					send_command_to_fvwm(commandBuffer,
						neighbor->window);
				}
			}
			AdhereGroup(neighbor);
		}
	}
}

static void DestroyWindow(Window w)
{
	ProxyWindow *proxy;
	ProxyWindow *prev;

	for (proxy=firstProxy, prev = NULL; proxy != NULL;
		prev = proxy, proxy=proxy->next)
	{
		if (proxy->proxy==w || proxy->window==w)
			break;
	}
	if (proxy == NULL)
	{
		return;
	}
	if (prev == NULL)
	{
		firstProxy = proxy->next;
	}
	else
	{
		prev->next = proxy->next;
	}
	if (selectProxy == proxy)
	{
		selectProxy = NULL;
	}
	if (enterProxy == proxy)
	{
		enterProxy = NULL;
	}
	if(!proxy->flags.is_iconified)
	{
		IsolateCheck(proxy,1);
	}
	CloseOneWindow(proxy);
	delete_ProxyWindow(proxy);

	return;
}

static unsigned int GetModifiers(void)
{
	Window root_return, child_return;
	int root_x_return, root_y_return;
	int win_x_return, win_y_return;
	unsigned int mask_return;

	if (FQueryPointer(
			dpy,rootWindow,&root_return,
			&child_return,
			&root_x_return,&root_y_return,
			&win_x_return,&win_y_return,
			&mask_return) == False)
	{
		/* pointer is on another screen - ignore */
	}

	/* mask_return
		0x01	shift
		0x02	caplock
		0x04	ctrl
		0x08	alt
		0x40	logo
	*/
	return mask_return;
}

static void StartProxies(void)
{
	if (are_windows_shown)
	{
		return;
	}

	held_modifiers=GetModifiers();
	enterProxy=NULL;
	selectProxy=NULL;

	if(action_list[PROXY_ACTION_MODIFIER_RELEASE])
		watching_modifiers=1;

	send_command_to_fvwm(action_list[PROXY_ACTION_SHOW], None);
	are_windows_shown = 1;
	CloseWindows();
	ReshuffleWindows();
	OpenWindows();

	return;
}

static void MarkProxy(ProxyWindow *new_proxy)
{
	ProxyWindow *old_proxy;

	old_proxy = selectProxy;
	selectProxy = new_proxy;
	if (selectProxy != old_proxy)
	{
		if (old_proxy != NULL)
		{
			DrawProxyBackground(old_proxy);
			DrawProxy(old_proxy);
		}
		if (selectProxy != NULL)
		{
			DrawProxyBackground(selectProxy);
			DrawProxy(selectProxy);
		}
	}
	if (old_proxy != NULL)
		send_command_to_fvwm(action_list[PROXY_ACTION_UNMARK],
				old_proxy->window);
	if (selectProxy != NULL)
		send_command_to_fvwm(action_list[PROXY_ACTION_MARK],
				selectProxy->window);

	if(showOnly && are_windows_shown)
	{
		ReshuffleWindows();
		selectProxy = new_proxy;
	}
	return;
}

static void HideProxies(void)
{
	if (!are_windows_shown)
	{
		return;
	}
	are_windows_shown = 0;
	CloseWindows();

	return;
}

static void SelectProxy(void)
{
	ProxyWindow *proxy;

	HideProxies();
	if (selectProxy)
		send_command_to_fvwm(action_list[PROXY_ACTION_SELECT],
				selectProxy->window);

	send_command_to_fvwm(action_list[PROXY_ACTION_HIDE], None);

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
		if (proxy==selectProxy)
		{
			startProxy=proxy;
			break;
		}

	selectProxy=NULL;

	return;
}

static void AbortProxies(void)
{
	HideProxies();
	send_command_to_fvwm(action_list[PROXY_ACTION_ABORT], None);
	selectProxy = NULL;

	return;
}

static void RotateIsolated(ProxyWindow *instigator,int direction)
{
	int found=0;
	ProxyWindow *proxy;
	ProxyWindow *first=NULL;
	ProxyWindow *adjacent=NULL;

#if PROXY_GROUP_DEBUG
	fprintf(stderr,"RotateIsolated %p %p %d\n",
		instigator,last_rotation_instigator,direction);
#endif

	/* rotation can lose focus, so we store it */
	if(!instigator || !instigator->group ||
		!instigator->flags.is_isolated)
	{
		instigator=last_rotation_instigator;
	}
	if(!instigator || !instigator->group ||
		!instigator->flags.is_isolated)
	{
		return;
	}
	last_rotation_instigator=instigator;

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		if(proxy->desk == instigator->desk &&
			proxy->group==instigator->group)
		{
			if(!proxy->flags.is_iconified)
			{
				if(direction<0 && adjacent)
				{
					break;
				}
				found=1;
			}
			else
			{
				if(direction<0)
				{
					adjacent=proxy;
				}
				else if(found)
				{
					adjacent=proxy;
					break;
				}
				else if(!first)
				{
					first=proxy;
				}
			}
		}
	}

	if(!adjacent)
	{
		adjacent=first;
	}
	if(adjacent)
	{
		sprintf(commandBuffer,"Iconify Off");
		send_command_to_fvwm(commandBuffer,adjacent->window);
	}

	return;
}

static void change_cset(int cset)
{
	if (cset == cset_normal || cset == cset_iconified)
	{
		ProxyWindow *proxy;

		for (proxy = firstProxy; proxy != NULL; proxy = proxy->next)
		{
			DrawProxyBackground(proxy);
		}
	}
	else if (cset == cset_select && selectProxy != NULL)
	{
		DrawProxyBackground(selectProxy);
	}

	return;
}

static void ProcessMessage(FvwmPacket* packet)
{
	unsigned long type = packet->type;
	unsigned long length = packet->size;
	unsigned long* body = packet->body;
	FvwmWinPacketBodyHeader *bh = (void *)body;
	ProxyWindow *proxy;
	int x=0;
	int y=0;
	int w=0;
	int h=0;
	int reshuffle=0;

	if(type!=M_RAISE_WINDOW && type!=M_LOWER_WINDOW && type!=M_RESTACK &&
		type!=M_FOCUS_CHANGE && type!=M_ICON_NAME &&
		type!=M_MINI_ICON && type!=M_STRING)
	{
		ClearRaised();
	}

	switch (type)
	{
	case M_CONFIGURE_WINDOW:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_CONFIGURE_WINDOW\n");
#endif
	case M_ADD_WINDOW:
#if PROXY_EVENT_DEBUG
		if(type!=M_CONFIGURE_WINDOW)
		{
			fprintf(stderr,
				"FvwmProxy ProcessMessage M_ADD_WINDOW\n");
		}
#endif
		ConfigureWindow(packet);
		break;
	case M_DESTROY_WINDOW:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_DESTROY_WINDOW\n");
#endif
		DestroyWindow(bh->w);
		break;
	case M_ICONIFY:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_ICONIFY\n");
#endif
		IconifyWindow(FindProxy(bh->w), 1);
		break;
	case M_DEICONIFY:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_DEICONIFY\n");
#endif
		IconifyWindow(FindProxy(bh->w), 0);
		break;
	case M_RAISE_WINDOW:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_RAISE_WINDOW\n");
#endif
		RaiseLowerGroup(bh->w,1);
		break;
	case M_LOWER_WINDOW:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_LOWER_WINDOW\n");
#endif
		RaiseLowerGroup(bh->w,0);
		break;
	case M_RESTACK:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_RESTACK\n");
#endif
		ReadStack(body,length);
		break;
	case M_WINDOW_NAME:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_WINDOW_NAME\n");
#endif
		proxy = FindProxy(bh->w);
		if (proxy != NULL)
		{
			if (proxy->name != NULL)
			{
				free(proxy->name);
			}
			proxy->name = xstrdup((char *)&body[3]);
#if PROXY_GROUP_DEBUG
			fprintf(stderr,"M_WINDOW_NAME %s\n",proxy->name);
#endif
			UpdateProxyGroup(proxy);
			UpdateProxyGroupForAll();
		}
		break;
	case M_ICON_NAME:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_ICON_NAME\n");
#endif
		proxy = FindProxy(bh->w);
		if (proxy != NULL)
		{
			if (proxy->iconname != NULL)
			{
				free(proxy->iconname);
			}
			proxy->iconname = xstrdup((char *)&body[3]);
#if PROXY_GROUP_DEBUG
			fprintf(stderr,"M_ICON_NAME %s\n",proxy->iconname);
#endif
/*			UpdateOneWindow(proxy);
*/
			DrawProxyBackground(proxy);
			DrawProxy(proxy);
		}
		break;
	case M_NEW_DESK:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_NEW_DESK\n");
#endif
		if (deskNumber!=body[0])
		{
			deskNumber=body[0];
			if (are_windows_shown)
			{
				ReshuffleWindows();
			}
		}
		break;
	case M_NEW_PAGE:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_NEW_PAGE\n");
#endif
		deskNumber=body[2];
		ReshuffleWindows();
		break;
	case M_MINI_ICON:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_MINI_ICON\n");
#endif
		proxy = FindProxy(bh->w);
		if (proxy != NULL)
		{
			proxy->picture.width=body[3];
			proxy->picture.height=body[4];
			proxy->picture.depth=body[5];
			proxy->picture.picture=body[6];
			proxy->picture.mask=body[7];
			proxy->picture.alpha=body[8];
			UpdateOneWindow(proxy);
		}
		break;
	case M_FOCUS_CHANGE:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_FOCUS_CHANGE\n");
#endif
	{
		focusWindow=bh->w;
		if(bh->w != 0)
		{
			last_rotation_instigator=NULL;
		}
		if(showOnly && !selectProxy)
		{
			reshuffle=1;
		}
	}
		break;
	case M_STRING:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_STRING\n");
#endif
	{
		char *message=(char*)&body[3];
		char *token;
		char *next;
		int prev;
		int isolate_check=0;

#if PROXY_EVENT_DEBUG
		fprintf(stderr, "M_STRING \"%s\"\n", message);
#endif

		token = PeekToken(message, &next);
		prev=(StrEquals(token, "Prev"));
		if (StrEquals(token, "Next") || prev)
		{
			ProxyWindow *lastSelect=selectProxy;
			ProxyWindow *newSelect=selectProxy;
			ProxyWindow *first=prev? lastProxy: firstProxy;

			/* auto-show if not already shown */
			if (!are_windows_shown)
				StartProxies();

			if (startProxy && startProxy->desk==deskNumber)
			{
				newSelect=startProxy;
				startProxy=NULL;
			}
			else
			{
				if (newSelect)
				{
					if (prev)
						newSelect=newSelect->prev;
					else
						newSelect=newSelect->next;
				}
				if (!newSelect)
					newSelect=first;
				while (newSelect!=lastSelect &&
						newSelect->desk!=deskNumber)
				{
					if (prev)
						newSelect=newSelect->prev;
					else
						newSelect=newSelect->next;
					if (!newSelect && lastSelect)
						newSelect=first;
				}
			}

			MarkProxy(newSelect);
		}
		else if (StrEquals(token, "Circulate"))
		{
			Window w;

			/* auto-show if not already shown */
			if (!are_windows_shown)
				StartProxies();

			w = (selectProxy) ? selectProxy->window : focusWindow;

			strcpy(commandBuffer,next);
			strcat(commandBuffer," SendToModule FvwmProxy Mark");
			if (next)
				SendFvwmPipe(fd,commandBuffer,w);
		}
		else if (StrEquals(token, "Show"))
		{
			StartProxies();
		}
		else if (StrEquals(token, "Hide"))
		{
			SelectProxy();
		}
		else if (StrEquals(token, "ShowToggle"))
		{
			if (are_windows_shown)
			{
				SelectProxy();
			}
			else
			{
				StartProxies();
			}
		}
		else if (StrEquals(token, "Abort"))
		{
			AbortProxies();
		}
		else if (StrEquals(token, "Mark"))
		{
#if 0
			Window w;

			if (next == NULL)
			{
				proxy = NULL;
			}
			else if (sscanf(next, "0x%x", (int *)&w) < 1)
			{
				proxy = NULL;
			}
			else
#endif
			{
				focusWindow=bh->w;
				proxy = FindProxy(bh->w);
			}
			MarkProxy(proxy);
		}
		else if (StrEquals(token, "Resolve"))
		{
			waiting_to_config=0;
			pending_do=0;
			ResolvePendingWindows();
			reshuffle=1;
		}
		else if (StrEquals(token, "Stamp"))
		{
			token = PeekToken(message, &next);
			sscanf(next,"%d%d%d%d",&x,&y,&w,&h);

			waiting_to_stamp=0;
			proxy = FindProxy(bh->w);
			if(proxy && w && h)
			{
				StoreStamp(bh->w,x,y,w,h);
			}
			reshuffle=1;
		}
		else if (StrEquals(token, "Undo"))
		{
			UndoStamp();
			reshuffle=1;
		}
		else if (StrEquals(token, "Redo"))
		{
			RedoStamp();
			reshuffle=1;
		}
		else if (StrEquals(token, "PrevIsolated"))
		{
			RotateIsolated(FindProxy(bh->w),-1);
		}
		else if (StrEquals(token, "NextIsolated"))
		{
			RotateIsolated(FindProxy(bh->w),1);
		}
		else if (StrEquals(token, "SoftToggle"))
		{
			proxy = FindProxy(bh->w);
			if(proxy)
			{
				proxy->flags.is_soft= proxy->group?
					!proxy->flags.is_soft: 0;
				isolate_check=1;

				DrawProxyBackground(proxy);
				DrawProxy(proxy);
			}
		}
		else if (StrEquals(token, "IsolateToggle"))
		{
			proxy = FindProxy(bh->w);
			if(proxy)
			{
				proxy->flags.is_isolated= proxy->group?
					!proxy->flags.is_isolated: 0;
				IsolateGroup(proxy,proxy->flags.is_isolated);
				isolate_check=1;

				DrawProxyBackground(proxy);
				DrawProxy(proxy);
				reshuffle=1;
			}
		}
		if(isolate_check)
		{
			IsolateCheck(proxy,0);
		}
		break;
	}
	case M_CONFIG_INFO:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy ProcessMessage M_CONFIG_INFO\n");
#endif
	{
		char *tline, *token;

		tline = (char*)&(body[3]);
		token = PeekToken(tline, &tline);
		if (StrEquals(token, "Colorset"))
		{
			int cset;
			cset = LoadColorset(tline);
			change_cset(cset);
		}
		break;
	}
	}

	if(reshuffle)
	{
		/* windows may have moved, so update proxy windows */
		if (are_windows_shown)
		{
			ReshuffleWindows();
		}
	}

	return;
}

static int My_XNextEvent(Display *dpy,XEvent *event)
{
	fd_set in_fdset;

	struct timeval timevalue,*timeout=&timevalue;
	timevalue.tv_sec = 0;
	timevalue.tv_usec = 100000;

	if (FPending(dpy))
	{
		FNextEvent(dpy,event);
		return 1;
	}

	FD_ZERO(&in_fdset);
	FD_SET(x_fd,&in_fdset);
	FD_SET(fd[1],&in_fdset);

	if ( fvwmSelect(fd_width, &in_fdset, 0, 0, timeout) > 0)
	{
		if (FD_ISSET(x_fd, &in_fdset))
		{
			if (FPending(dpy))
			{
				FNextEvent(dpy,event);
				return 1;
			}
		}

		if (FD_ISSET(fd[1], &in_fdset))
		{
			FvwmPacket* packet = ReadFvwmPacket(fd[1]);
			if (!packet)
			{
				exit(0);
			}

			ProcessMessage(packet);
		}
	}

	return 0;
}

static void DispatchEvent(XEvent *pEvent)
{
	Window window=pEvent->xany.window;
	ProxyWindow *proxy;
	int dx,dy;

	switch(pEvent->xany.type)
	{
	case Expose:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy DispatchEvent Expose\n");
#endif
		proxy = FindProxy(window);
		if (proxy != NULL)
		{
			DrawWindow(
				proxy, pEvent->xexpose.x, pEvent->xexpose.y,
				pEvent->xexpose.width, pEvent->xexpose.height);
		}
		break;
	case ButtonPress:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy DispatchEvent ButtonPress\n");
#endif
		proxy = FindProxy(window);
		if (proxy)
		{
			int button=pEvent->xbutton.button;
			int wx=pEvent->xbutton.x;
			int wy=pEvent->xbutton.y;
			if (button >= 1 &&
				button <= NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
			{
				if(wy < slotSpace+slotHeight)
				{
					int index=(wx-slotSpace)/
						(slotWidth+slotSpace)+1;
					int group=index-groupSlot+1;

					if(index<numSlots)
					{
						char* action=slot_action_list[
							button-1][index];
						if(action)
						{
							SendFvwmPipe(fd,action,
								proxy->window);
						}
					}
					if(group && group<=groupCount)
					{
						SetGroup(proxy,
							proxy->group==group?
							0: group);
						DrawProxyBackground(proxy);
						DrawProxy(proxy);
					}
				}
				else
				{
					SendFvwmPipe(fd,
						action_list[PROXY_ACTION_CLICK
							+ button-1],
						proxy->window);
				}
			}
		}
		mousex=pEvent->xbutton.x_root;
		mousey=pEvent->xbutton.y_root;
		break;
	case MotionNotify:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy DispatchEvent MotionNotify\n");
#endif
		proxy = FindProxy(window);
		dx=pEvent->xbutton.x_root-mousex;
		dy=pEvent->xbutton.y_root-mousey;
		if (proxy && proxyMove)
		{
			sprintf(commandBuffer,"Silent Move w+%dp w+%dp",dx,dy);
			send_command_to_fvwm(commandBuffer,proxy->window);
		}

		mousex=pEvent->xbutton.x_root;
		mousey=pEvent->xbutton.y_root;
		break;
	case EnterNotify:
#if PROXY_EVENT_DEBUG
		fprintf(stderr, "FvwmProxy DispatchEvent EnterNotify\n");
#endif
		proxy = FindProxy(pEvent->xcrossing.window);
		if (pEvent->xcrossing.mode == NotifyNormal)
		{
			MarkProxy(proxy);
			enterProxy = proxy;
		}
		else if (pEvent->xcrossing.mode == NotifyUngrab &&
			 proxy != NULL && proxy != selectProxy &&
			 proxy != enterProxy)
		{
			MarkProxy(proxy);
			enterProxy = proxy;
		}
		break;
	default:
		fprintf(stderr, "Unrecognized XEvent %d\n", pEvent->xany.type);
		break;
	}

	return;
}

static void Loop(int *fd)
{
	XEvent event;
	long result;

	while (1)
	{
		if ((result=My_XNextEvent(dpy,&event))==1)
		{
			DispatchEvent(&event);
		}

#if PROXY_KEY_POLLING
		if(are_windows_shown && watching_modifiers)
		{
			unsigned int mask_return=GetModifiers();
			if(!(mask_return&watched_modifiers))
			{
				watching_modifiers=0;
				send_command_to_fvwm(
					action_list
					[PROXY_ACTION_MODIFIER_RELEASE],
					None);
			}
		}
#endif
	}
}

/* ---------------------------- interface functions ------------------------- */

int main(int argc, char **argv)
{
	char *titles[1];

	if (argc < 6)
	{
		fprintf(
			stderr,
			"FvwmProxy should only be executed by fvwm!\n");
		exit(1);
	}

	FlocaleInit(LC_CTYPE, "", "", "FvwmProxy");
	MyName = GetFileNameFromPath(argv[0]);

	fd[0] = atoi(argv[1]);
	fd[1] = atoi(argv[2]);

	originalXIOErrorHandler=XSetIOErrorHandler(myXIOErrorHandler);
	originalXErrorHandler=XSetErrorHandler(myXErrorHandler);

	if (!(dpy=XOpenDisplay(NULL)))
	{
		fprintf(stderr,"can't open display\n");
		exit (1);
	}
	titles[0]="FvwmProxy";
	if (XStringListToTextProperty(titles,1,&windowName) == 0)
	{
		fprintf(stderr,"Proxy_CreateBar() could not allocate space"
			" for window title");
	}

	flib_init_graphics(dpy);
	FlocaleAllocateWinString(&FwinString);
	screen = DefaultScreen(dpy);
	rootWindow = RootWindow(dpy,screen);
	xgcv.plane_mask=AllPlanes;
	miniIconGC=fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);
	fg_gc = fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);
	hi_gc = fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);
	sh_gc = fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);

	x_fd = XConnectionNumber(dpy);
	fd_width = GetFdWidth();

	SetMessageMask(
		fd, M_STRING| M_RAISE_WINDOW| M_LOWER_WINDOW|
		M_CONFIGURE_WINDOW| M_ADD_WINDOW| M_FOCUS_CHANGE|
		M_DESTROY_WINDOW| M_NEW_DESK| M_NEW_PAGE| M_ICON_NAME|
		M_WINDOW_NAME| M_MINI_ICON| M_ICONIFY| M_DEICONIFY|
		M_CONFIG_INFO| M_END_CONFIG_INFO| M_RESTACK);

	if (parse_options() == False)
		exit(1);
	stampQueue=xmalloc(sizeof(GeometryStamp) * stampLimit);
	if ((Ffont = FlocaleLoadFont(dpy, font_name, MyName)) == NULL)
	{
		fprintf(stderr,"%s: Couldn't load font \"%s\". Exiting!\n",
			MyName, font_name);
		exit(1);
	}
	if (Ffont->font != NULL)
	{
		XSetFont(dpy,fg_gc,Ffont->font->fid);
	}
	Ffont_small=NULL;
	if(small_font_name)
	{
		if((Ffont_small =
			FlocaleLoadFont(dpy, small_font_name, MyName)) == NULL)
		{
			fprintf(stderr,
				"%s: Couldn't load small font \"%s\"\n",
				MyName, small_font_name);
		}
	}
	if (Ffont_small != NULL && Ffont_small->font != NULL)
	{
		XSetFont(dpy,hi_gc,Ffont_small->font->fid);
	}
	SendInfo(fd,"Send_WindowList",0);
	SendFinishedStartupNotification(fd);
	Loop(fd);

	free(stampQueue);
	return 0;
}
