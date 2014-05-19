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
/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#include <limits.h>

#include "libs/fvwmlib.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/ColorUtils.h"
#include "libs/FScreen.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "misc.h"
#include "move_resize.h"
#include "screen.h"
#include "geometry.h"
#include "read.h"
#include "virtual.h"
#include "colorset.h"
#include "schedule.h"
#include "infostore.h"
#include "libs/FGettext.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "libs/Fsvg.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

extern char const * const Fvwm_VersionInfo;

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static char *partial_function_vars[] =
{
	"bg.cs",
	"desk.name",
	"fg.cs",
	"fgsh.cs",
	"gt.",
	"hilight.cs",
	"infostore.",
	"shadow.cs",
	NULL
};

static char *function_vars[] =
{
	"cond.rc",
	"cw.height",
	"cw.width",
	"cw.x",
	"cw.y",
	"desk.height",
	"desk.n",
	"desk.pagesx",
	"desk.pagesy",
	"desk.width",
	"func.context",
	"i.height",
	"i.width",
	"i.x",
	"i.y",
	"ip.height",
	"ip.width",
	"ip.x",
	"ip.y",
	"it.height",
	"it.width",
	"it.x",
	"it.y",
	"page.nx",
	"page.ny",
	"pointer.cx",
	"pointer.cy",
	"pointer.wx",
	"pointer.wy",
	"pointer.x",
	"pointer.y",
	"pointer.screen",
	"schedule.last",
	"schedule.next",
	"screen",
	"version.info",
	"version.line",
	"version.num",
	"vp.height",
	"vp.width",
	"vp.x",
	"vp.y",
	"w.class",
	"w.height",
	"w.iconname",
	"w.iconfile",
	"w.miniiconfile",
	"w.iconfile.svgopts",
	"w.miniiconfile.svgopts",
	"w.id",
	"w.name",
	"w.resource",
	"w.visiblename",
	"w.width",
	"w.x",
	"w.y",
	"w.desk",
	"w.layer",
	"w.screen",
	/* ewmh working area */
	"wa.height",
	"wa.width",
	"wa.x",
	"wa.y",
	/* ewmh dynamic working area */
	"dwa.height",
	"dwa.width",
	"dwa.x",
	"dwa.y",
	NULL
};

enum
{
	VAR_BG_CS,
	VAR_DESK_NAME,
	VAR_FG_CS,
	VAR_FGSH_CS,
	VAR_GT_,
	VAR_HILIGHT_CS,
	VAR_INFOSTORE_,
	VAR_SHADOW_CS
} partial_extended_vars;

enum
{
	VAR_COND_RC,
	VAR_CW_HEIGHT,
	VAR_CW_WIDTH,
	VAR_CW_X,
	VAR_CW_Y,
	VAR_DESK_HEIGHT,
	VAR_DESK_N,
	VAR_DESK_PAGESX,
	VAR_DESK_PAGESY,
	VAR_DESK_WIDTH,
	VAR_FUNC_CONTEXT,
	VAR_I_HEIGHT,
	VAR_I_WIDTH,
	VAR_I_X,
	VAR_I_Y,
	VAR_IP_HEIGHT,
	VAR_IP_WIDTH,
	VAR_IP_X,
	VAR_IP_Y,
	VAR_IT_HEIGHT,
	VAR_IT_WIDTH,
	VAR_IT_X,
	VAR_IT_Y,
	VAR_PAGE_NX,
	VAR_PAGE_NY,
	VAR_POINTER_CX,
	VAR_POINTER_CY,
	VAR_POINTER_WX,
	VAR_POINTER_WY,
	VAR_POINTER_X,
	VAR_POINTER_Y,
	VAR_POINTER_SCREEN,
	VAR_SCHEDULE_LAST,
	VAR_SCHEDULE_NEXT,
	VAR_SCREEN,
	VAR_VERSION_INFO,
	VAR_VERSION_LINE,
	VAR_VERSION_NUM,
	VAR_VP_HEIGHT,
	VAR_VP_WIDTH,
	VAR_VP_X,
	VAR_VP_Y,
	VAR_W_CLASS,
	VAR_W_HEIGHT,
	VAR_W_ICONNAME,
	VAR_W_ICONFILE,
	VAR_W_MINIICONFILE,
	VAR_W_ICONFILE_SVGOPTS,
	VAR_W_MINIICONFILE_SVGOPTS,
	VAR_W_ID,
	VAR_W_NAME,
	VAR_W_RESOURCE,
	VAR_W_VISIBLE_NAME,
	VAR_W_WIDTH,
	VAR_W_X,
	VAR_W_Y,
	VAR_W_DESK,
	VAR_W_LAYER,
	VAR_W_SCREEN,
	/* ewmh working area */
	VAR_WA_HEIGHT,
	VAR_WA_WIDTH,
	VAR_WA_X,
	VAR_WA_Y,
	/* ewmh dynamic working area */
	VAR_DWA_HEIGHT,
	VAR_DWA_WIDTH,
	VAR_DWA_X,
	VAR_DWA_Y
} extended_vars;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

int __eae_parse_range(char *input, int *lower, int *upper)
{
	int rc;
	int n;

	*lower = 0;
	*upper = INT_MAX;
	if (*input == '*')
	{
		return 0;
	}
	if (!isdigit(*input))
	{
		return -1;
	}
	rc = sscanf(input, "%d-%d%n", lower, upper, &n);
	if (rc < 2)
	{
		rc = sscanf(input, "%d%n", lower, &n);
		if (rc < 1)
		{
			/* not a positional argument */
			return -1;
		}
		if (input[n] == '-')
		{
			/* $[n- */
			n++;
		}
		else
		{
			/* $[n */
			*upper = *lower;
		}
	}
	input += n;
	if (*input != 0)
	{
		/* trailing characters - not good */
		return -1;
	}
	if (*upper < *lower)
	{
		/* the range is reverse - not good */
		return -1;
	}

	return 0;
}

static signed int expand_args_extended(
	char *input, char *argument_string, char *output)
{
	int rc;
	int lower;
	int upper;
	int i;
	size_t len;

	rc = __eae_parse_range(input, &lower, &upper);
	if (rc == -1)
	{
		return -1;
	}
	/* Skip to the start of the requested argument range */
	if (lower > 0)
	{
		argument_string = SkipNTokens(argument_string, lower);
	}
	if (!argument_string)
	{
		/* replace with empty string */
		return 0;
	}
	/* TODO: optimise handling of $[0] to $[9] which have already been
	 * parsed */
	for (i = lower, len = 0; i <= upper; i++)
	{
		char *token;
		size_t tlen;

		token = PeekToken(argument_string, &argument_string);
		if (token == NULL)
		{
			break;
		}
		/* copy the token */
		if (i > lower)
		{
			if (output != NULL)
			{
				*output = ' ';
				output++;
			}
			len++;
		}
		tlen = strlen(token);
		if (output != NULL && tlen > 0)
		{
			memcpy(output, token, tlen);
			output += tlen;
		}
		len += tlen;
	}

	return (int)len;
}

static signed int expand_vars_extended(
	char *var_name, char *output, cond_rc_t *cond_rc,
	const exec_context_t *exc)
{
	char *rest;
	char dummy[64] = "\0";
	char *target = (output) ? output : dummy;
	int cs = -1;
	int n;
	int i;
	int l;
	int x;
	int y;
	Pixel pixel = 0;
	int val = -12345678;
	const char *string = NULL;
	char *allocated_string = NULL;
	char *quoted_string = NULL;
	Bool should_quote = False;
	Bool is_numeric = False;
	Bool is_target = False;
	Bool is_x;
	Window context_w = Scr.Root;
	FvwmWindow *fw = exc->w.fw;
	signed int len = -1;

	/* allow partial matches for *.cs, gt, ... etc. variables */
	switch ((i = GetTokenIndex(var_name, partial_function_vars, -1, &rest)))
	{
	case VAR_FG_CS:
	case VAR_BG_CS:
	case VAR_HILIGHT_CS:
	case VAR_SHADOW_CS:
	case VAR_FGSH_CS:
		if (!isdigit(*rest) || (*rest == '0' && *(rest + 1) != 0))
		{
			/* not a non-negative integer without leading zeros */
			return -1;
		}
		if (sscanf(rest, "%d%n", &cs, &n) < 1)
		{
			return -1;
		}
		if (*(rest + n) != 0)
		{
			/* trailing characters */
			return -1;
		}
		if (cs < 0)
		{
			return -1;
		}
		alloc_colorset(cs);
		switch (i)
		{
		case VAR_FG_CS:
			pixel = Colorset[cs].fg;
			break;
		case VAR_BG_CS:
			pixel = Colorset[cs].bg;
			break;
		case VAR_HILIGHT_CS:
			pixel = Colorset[cs].hilite;
			break;
		case VAR_SHADOW_CS:
			pixel = Colorset[cs].shadow;
			break;
		case VAR_FGSH_CS:
			pixel = Colorset[cs].fgsh;
			break;
		}
		is_target = True;
		len = pixel_to_color_string(dpy, Pcmap, pixel, target, False);
		goto GOT_STRING;
	case VAR_GT_:
		if (rest == NULL)
		{
			return -1;
		}
		string = _(rest);
		goto GOT_STRING;
	case VAR_INFOSTORE_:
		if (rest == NULL)
			return -1;

		if ((string = get_metainfo_value(rest)) == NULL)
			return -1;

		goto GOT_STRING;
	case VAR_DESK_NAME:
		if (sscanf(rest, "%d%n", &cs, &n) < 1)
		{
			return -1;
		}
		if (*(rest + n) != 0)
		{
			/* trailing characters */
			return -1;
		}
		string = GetDesktopName(cs);
		if (string == NULL)
		{
			const char *ddn = _("Desk");
			/* TA:  FIXME!  xasprintf() */
			allocated_string = xmalloc(19 + strlen(ddn));
			sprintf(allocated_string, "%s %i", ddn, cs);
			string = allocated_string;
		}
		goto GOT_STRING;
	default:
		break;
	}

	/* only exact matches for all other variables */
	switch ((i = GetTokenIndex(var_name, function_vars, 0, &rest)))
	{
	case VAR_DESK_N:
		is_numeric = True;
		val = Scr.CurrentDesk;
		break;
	case VAR_DESK_WIDTH:
		is_numeric = True;
		val = Scr.VxMax + Scr.MyDisplayWidth;
		break;
	case VAR_DESK_HEIGHT:
		is_numeric = True;
		val = Scr.VyMax + Scr.MyDisplayHeight;
		break;
	case VAR_DESK_PAGESX:
		is_numeric = True;
		val = (int)(Scr.VxMax / Scr.MyDisplayWidth) + 1;
		break;
	case VAR_DESK_PAGESY:
		is_numeric = True;
		val = (int)(Scr.VyMax / Scr.MyDisplayHeight) + 1;
		break;
	case VAR_VP_X:
		is_numeric = True;
		val = Scr.Vx;
		break;
	case VAR_VP_Y:
		is_numeric = True;
		val = Scr.Vy;
		break;
	case VAR_VP_WIDTH:
		is_numeric = True;
		val = Scr.MyDisplayWidth;
		break;
	case VAR_VP_HEIGHT:
		is_numeric = True;
		val = Scr.MyDisplayHeight;
		break;
	case VAR_WA_HEIGHT:
		is_numeric = True;
		val = Scr.Desktops->ewmh_working_area.height;
		break;
	case VAR_WA_WIDTH:
		is_numeric = True;
		val = Scr.Desktops->ewmh_working_area.width;
		break;
	case VAR_WA_X:
		is_numeric = True;
		val = Scr.Desktops->ewmh_working_area.x;
		break;
	case VAR_WA_Y:
		is_numeric = True;
		val = Scr.Desktops->ewmh_working_area.y;
		break;
	case VAR_DWA_HEIGHT:
		is_numeric = True;
		val = Scr.Desktops->ewmh_dyn_working_area.height;
		break;
	case VAR_DWA_WIDTH:
		is_numeric = True;
		val = Scr.Desktops->ewmh_dyn_working_area.width;
		break;
	case VAR_DWA_X:
		is_numeric = True;
		val = Scr.Desktops->ewmh_dyn_working_area.x;
		break;
	case VAR_DWA_Y:
		is_numeric = True;
		val = Scr.Desktops->ewmh_dyn_working_area.y;
		break;
	case VAR_PAGE_NX:
		is_numeric = True;
		val = (int)(Scr.Vx / Scr.MyDisplayWidth);
		break;
	case VAR_PAGE_NY:
		is_numeric = True;
		val = (int)(Scr.Vy / Scr.MyDisplayHeight);
		break;
	case VAR_W_ID:
		if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
		{
			is_target = True;
			sprintf(target, "0x%x", (int)FW_W(fw));
		}
		break;
	case VAR_W_NAME:
		if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
		{
			string = fw->name.name;
			should_quote = True;
		}
		break;
	case VAR_W_ICONNAME:
		if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
		{
			string = fw->icon_name.name;
			should_quote = True;
		}
		break;
	case VAR_W_ICONFILE:
	case VAR_W_MINIICONFILE:
		if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
		{
			char *t;

			t = (i == VAR_W_ICONFILE) ?
				fw->icon_bitmap_file : fw->mini_pixmap_file;
			/* expand the path if possible */
			allocated_string = PictureFindImageFile(t, NULL, R_OK);
			if (allocated_string == NULL)
			{
				string = t;
			}
			else if (USE_SVG && *allocated_string == ':' &&
				 (string = strchr(allocated_string + 1, ':')))
			{
				string++;
			}
			else
			{
				string = allocated_string;
			}
		}
		break;
	case VAR_W_ICONFILE_SVGOPTS:
	case VAR_W_MINIICONFILE_SVGOPTS:
		if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
		{
			char *t;

			if (!USE_SVG)
			{
				return -1;
			}
			t = (i == VAR_W_ICONFILE_SVGOPTS) ?
				fw->icon_bitmap_file : fw->mini_pixmap_file;
			/* expand the path if possible */
			allocated_string = PictureFindImageFile(t, NULL, R_OK);
			string = allocated_string;
			if (string && *string == ':' &&
			    (t = strchr(string + 1, ':')))
			{
				*t = 0;
			}
			else
			{
				string = "";
			}
		}
		break;
	case VAR_W_CLASS:
		if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
		{
			string = fw->class.res_class;
			should_quote = True;
		}
		break;
	case VAR_W_RESOURCE:
		if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
		{
			string = fw->class.res_name;
			should_quote = True;
		}
		break;
	case VAR_W_VISIBLE_NAME:
		if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
		{
			string = fw->visible_name;
			should_quote = True;
		}
		break;
	case VAR_W_X:
	case VAR_W_Y:
	case VAR_W_WIDTH:
	case VAR_W_HEIGHT:
		if (!fw || IS_ICONIFIED(fw) || IS_EWMH_DESKTOP(FW_W(fw)))
		{
			return -1;
		}
		else
		{
			rectangle g;

			is_numeric = True;
			get_unshaded_geometry(fw, &g);
			switch (i)
			{
			case VAR_W_X:
				val = g.x;
				break;
			case VAR_W_Y:
				val = g.y;
				break;
			case VAR_W_WIDTH:
				val = g.width;
				break;
			case VAR_W_HEIGHT:
				val = g.height;
				break;
			default:
				return -1;
			}
		}
		break;
	case VAR_CW_X:
	case VAR_CW_Y:
	case VAR_CW_WIDTH:
	case VAR_CW_HEIGHT:
		if (!fw || IS_ICONIFIED(fw) || IS_EWMH_DESKTOP(FW_W(fw)))
		{
			return -1;
		}
		else
		{
			rectangle g;

			is_numeric = True;
			get_client_geometry(fw, &g);
			switch (i)
			{
			case VAR_CW_X:
				val = g.x;
				break;
			case VAR_CW_Y:
				val = g.y;
				break;
			case VAR_CW_WIDTH:
				val = g.width;
				break;
			case VAR_CW_HEIGHT:
				val = g.height;
				break;
			default:
				return -1;
			}
		}
		break;
	case VAR_IT_X:
	case VAR_IT_Y:
	case VAR_IT_WIDTH:
	case VAR_IT_HEIGHT:
		if (!fw || IS_EWMH_DESKTOP(FW_W(fw)))
		{
			return -1;
		}
		else
		{
			rectangle g;

			if (get_visible_icon_title_geometry(fw, &g) == False)
			{
				return -1;
			}
			is_numeric = True;
			switch (i)
			{
			case VAR_IT_X:
				val = g.x;
				break;
			case VAR_IT_Y:
				val = g.y;
				break;
			case VAR_IT_WIDTH:
				val = g.width;
				break;
			case VAR_IT_HEIGHT:
				val = g.height;
				break;
			default:
				return -1;
			}
		}
		break;
	case VAR_IP_X:
	case VAR_IP_Y:
	case VAR_IP_WIDTH:
	case VAR_IP_HEIGHT:
		if (!fw || IS_EWMH_DESKTOP(FW_W(fw)))
		{
			return -1;
		}
		else
		{
			rectangle g;

			if (get_visible_icon_picture_geometry(fw, &g) == False)
			{
				return -1;
			}
			is_numeric = True;
			switch (i)
			{
			case VAR_IP_X:
				val = g.x;
				break;
			case VAR_IP_Y:
				val = g.y;
				break;
			case VAR_IP_WIDTH:
				val = g.width;
				break;
			case VAR_IP_HEIGHT:
				val = g.height;
				break;
			default:
				return -1;
			}
		}
		break;
	case VAR_I_X:
	case VAR_I_Y:
	case VAR_I_WIDTH:
	case VAR_I_HEIGHT:
		if (!fw || IS_EWMH_DESKTOP(FW_W(fw)))
		{
			return -1;
		}
		else
		{
			rectangle g;

			if (get_visible_icon_geometry(fw, &g) == False)
			{
				return -1;
			}
			is_numeric = True;
			switch (i)
			{
			case VAR_I_X:
				val = g.x;
				break;
			case VAR_I_Y:
				val = g.y;
				break;
			case VAR_I_WIDTH:
				val = g.width;
				break;
			case VAR_I_HEIGHT:
				val = g.height;
				break;
			default:
				return -1;
			}
		}
		break;
	case VAR_W_DESK:
		if (!fw || IS_EWMH_DESKTOP(FW_W(fw)))
		{
			return -1;
		}
		is_numeric = True;
		if (is_window_sticky_across_desks(fw))
		{
			val = Scr.CurrentDesk;
		}
		else
		{
			val = fw->Desk;
		}
		break;
	case VAR_W_LAYER:
		if (!fw || IS_EWMH_DESKTOP(FW_W(fw)))
		{
			return -1;
		}
		is_numeric = True;
		val = fw->layer;
		break;
	case VAR_W_SCREEN:
		if (!fw || IS_EWMH_DESKTOP(FW_W(fw))) {
			return -1;
		} else {
			is_numeric = True;

			rectangle	g;
			get_unshaded_geometry(fw, &g);

			val = FScreenOfPointerXY(g.x, g.y);
		}
		break;
	case VAR_SCREEN:
		is_numeric = True;
		val = Scr.screen;
		break;
	case VAR_SCHEDULE_LAST:
		is_numeric = True;
		val = squeue_get_last_id();
		break;
	case VAR_SCHEDULE_NEXT:
		is_numeric = True;
		val = squeue_get_next_id();
		break;
	case VAR_COND_RC:
		if (cond_rc == NULL)
		{
			return -1;
		}
		switch (cond_rc->rc)
		{
		case COND_RC_OK:
		case COND_RC_NO_MATCH:
		case COND_RC_ERROR:
		case COND_RC_BREAK:
			val = (int)(cond_rc->rc);
			break;
		default:
			return -1;
		}
		is_numeric = True;
		break;
	case VAR_POINTER_X:
	case VAR_POINTER_Y:
		if (is_numeric == False)
		{
			is_numeric = True;
			context_w = Scr.Root;
		}
		/* fall through */
	case VAR_POINTER_WX:
	case VAR_POINTER_WY:
		if (is_numeric == False)
		{
			if (!fw || IS_ICONIFIED(fw)
			    || IS_EWMH_DESKTOP(FW_W(fw)))
			{
				return -1;
			}
			is_numeric = True;
			context_w = FW_W_FRAME(fw);
		}
		/* fall through */
	case VAR_POINTER_CX:
	case VAR_POINTER_CY:
		if (is_numeric == False)
		{
			if (!fw || IS_ICONIFIED(fw) || IS_SHADED(fw)
			    || IS_EWMH_DESKTOP(FW_W(fw)))
			{
				return -1;
			}
			is_numeric = True;
			context_w = FW_W(fw);
		}
		is_x = False;
		switch (i)
		{
		case VAR_POINTER_X:
		case VAR_POINTER_WX:
		case VAR_POINTER_CX:
			is_x = True;
		}
		if (FQueryPointer(dpy, context_w, &JunkRoot, &JunkChild,
				  &JunkX, &JunkY, &x, &y, &JunkMask) == False)
		{
			/* pointer is on a different screen, don't expand */
			return -1;
		}
		val = (is_x) ? x : y;
		break;
	case VAR_POINTER_SCREEN:
		FQueryPointer(dpy, context_w, &JunkRoot, &JunkChild,
				&JunkX, &JunkY, &x, &y, &JunkMask);
		string = FScreenOfPointerXY(x, y);
		should_quote = True;
		break;

	case VAR_VERSION_NUM:
		string = VERSION;
		break;
	case VAR_VERSION_INFO:
		string = VERSIONINFO;
		break;
	case VAR_VERSION_LINE:
		string = Fvwm_VersionInfo;
		break;
	case VAR_FUNC_CONTEXT:
		is_target = True;
		target[0] = wcontext_wcontext_to_char(exc->w.wcontext);
		target[1] = '\0';
		break;
	default:
		/* unknown variable - try to find it in the environment */
		string = getenv(var_name);
		if (!string)
		{
			/* Replace it with unexpanded variable. This is needed
			 * since var_name might have been expanded */
			l = strlen(var_name) + 3;
			if (output)
			{
				strcpy(output, "$[");
				strcpy(output + 2, var_name);
				output[l - 1] = ']';
				output[l] = 0;
			}
			return l;
		}
	}

GOT_STRING:
	if (is_numeric)
	{
		is_target = True;
		sprintf(target, "%d", val);
	}
	if (is_target)
	{
		string = target;
	}
	else
	{
		if (!string)
		{
			return -1;
		}
		if (output)
		{
			strcpy(output, string);
		}
	}
	if (len < 0)
	{
		len = strlen(string);
	}
	if (should_quote)
	{
		quoted_string = xmalloc(len * 2 + 3);
		len = QuoteString(quoted_string, string) - quoted_string;
		if (output)
		{
			strcpy(output, quoted_string);
		}
		free(quoted_string);
	}
	if (allocated_string)
	{
		free(allocated_string);
	}
	return len;
}

/* ---------------------------- interface functions ------------------------ */

char *expand_vars(
	char *input, char *arguments[], Bool addto, Bool ismod,
	cond_rc_t *cond_rc, const exec_context_t *exc)
{
	int l, i, l2, n, k, j, m;
	int xlen, xlevel;
	Bool name_has_dollar;
	char *out;
	char *var;
	const char *string = NULL;
	Bool is_string = False;
	FvwmWindow *fw = exc->w.fw;

	l = strlen(input);
	l2 = l;

	if (input[0] == '+' && Scr.last_added_item.type == ADDED_FUNCTION)
	{
		addto = 1;
	}

	/* Calculate best guess at length of expanded string */
	i = 0;
	while (i < l)
	{
		if (input[i] == '$' && (!ismod || !isalpha(input[i + 1])))
		{
			switch (input[i + 1])
			{
			case '$':
				/* skip the second $, it is not a part of
				 * variable */
				i++;
				break;
			case '[':
				/* extended variables */
				m = i + 2;
				var = &input[m];
				xlevel = 1;
				name_has_dollar = False;
				while (m < l && xlevel && input[m])
				{
					/* handle nested variables */
					if (input[m] == ']')
					{
						xlevel--;
					}
					else if (input[m] == '[')
					{
						xlevel++;
					}
					else if (input[m] == '$')
					{
						name_has_dollar = True;
					}
					if (xlevel)
					{
						m++;
					}
				}
				if (input[m] == ']')
				{
					input[m] = 0;
					/* handle variable name */
					k = strlen(var);
					if (addto)
					{
						i += k + 2;
						input[m] = ']';
						break;
					}
					if (name_has_dollar)
					{
						var = expand_vars(
							var, arguments, addto,
							ismod, cond_rc, exc);
					}
					xlen = expand_args_extended(
						var, arguments ? arguments[0] :
						NULL, NULL);
					if (xlen < 0)
					{
						xlen = expand_vars_extended(
							var, NULL, cond_rc,
							exc);
					}
					if (name_has_dollar)
					{
						free(var);
					}
					if (xlen >= 0)
					{
						l2 += xlen - (k + 2);
					}
					i += k + 2;
					input[m] = ']';
				}
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '*':
				if (input[i + 1] == '*')
				{
					n = 0;
				}
				else
				{
					n = input[i + 1] - '0' + 1;
				}
				if (arguments[n] != NULL)
				{
					l2 += strlen(arguments[n]) - 2;
					i++;
				}
				break;
			case '.':
				string = get_current_read_dir();
				break;
			case 'w':
			case 'd':
			case 'x':
			case 'y':
				l2 += 16;
				i++;
				break;
			case 'c':
			case 'r':
			case 'n':
				if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
				{
					switch(input[i + 1])
					{
					case 'c':
						if (fw->class.res_class &&
						    fw->class.res_class[0])
						{
							string = fw->class.
								res_class;
						}
						break;
					case 'r':
						if (fw->class.res_name &&
						    fw->class.res_name[0])
						{
							string = fw->class.
								res_name;
						}
						break;
					case 'n':
						if (fw->name.name &&
						    fw->name.name[0])
						{
							string = fw->name.name;
						}
						break;
					}
				}
				break;
			case 'v':
				if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
				{
					switch(input[i + 1])
					{
						case 'v':
							if(fw->visible_name)
							{
								string = fw->visible_name;
							}
							break;
					}
				}

				if (Fvwm_VersionInfo)
				{
					l2 += strlen(Fvwm_VersionInfo) + 2;
				}
				break;
			}
			if (string)
			{
				for (k = 0; string[k] != 0; k++, l2++)
				{
					if (string[k] == '\'')
					{
						l2++;
					}
				}
				string = NULL;
			}

		}
		i++;
	}

	/* Actually create expanded string */
	i = 0;
	out = xmalloc(l2 + 1);
	j = 0;
	while (i < l)
	{
		if (input[i] == '$' && (!ismod || !isalpha(input[i + 1])))
		{
			switch (input[i + 1])
			{
			case '[':
				/* extended variables */
				if (addto)
				{
					/* Don't expand these in an 'AddToFunc'
					 * command */
					out[j++] = input[i];
					break;
				}
				m = i + 2;
				var = &input[m];
				xlevel = 1;
				name_has_dollar = False;
				while (m < l && xlevel && input[m])
				{
					/* handle nested variables */
					if (input[m] == ']')
					{
						xlevel--;
					}
					else if (input[m] == '[')
					{
						xlevel++;
					}
					else if (input[m] == '$')
					{
						name_has_dollar = True;
					}
					if (xlevel)
					{
						m++;
					}
				}
				if (input[m] == ']')
				{
					input[m] = 0;
					/* handle variable name */
					k = strlen(var);
					if (name_has_dollar)
					{
						var = expand_vars(
							var, arguments,	addto,
							ismod, cond_rc,	exc);
					}
					xlen = expand_args_extended(
						var, arguments ?
						arguments[0] : NULL,
						&out[j]);
					if (xlen < 0)
					{
						xlen = expand_vars_extended(
							var, &out[j], cond_rc,
							exc);
					}
					if (name_has_dollar)
					{
						free(var);
					}
					input[m] = ']';
					if (xlen >= 0)
					{
						j += xlen;
						i += k + 2;
					}
					else
					{
						/* copy the whole string in
						 * square brackets */
						for ( ; i <= m; i++, j++)
						{
							out[j] = input[i];
						}
						i--;
					}
				}
				else
				{
					out[j++] = input[i];
				}
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '*':
				if (input[i + 1] == '*')
				{
					n = 0;
				}
				else
				{
					n = input[i + 1] - '0' + 1;
				}
				if (arguments[n] != NULL)
				{
					for (k = 0; arguments[n][k]; k++)
					{
						out[j++] = arguments[n][k];
					}
					i++;
				}
				else if (addto == 1)
				{
					out[j++] = '$';
				}
				else
				{
					i++;
				}
				break;
			case '.':
				string = get_current_read_dir();
				is_string = True;
				break;
			case 'w':
				if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
				{
					fvwm_msg(OLD, "expand_vars",
						"Use $[w.id] instead of $w");
					sprintf(&out[j], "0x%x",
						(int)FW_W(fw));
				}
				else
				{
					sprintf(&out[j], "$w");
				}
				j += strlen(&out[j]);
				i++;
				break;
			case 'd':
				fvwm_msg(OLD, "expand_vars",
					"Use $[desk.n] instead of $d");
				sprintf(&out[j], "%d", Scr.CurrentDesk);
				j += strlen(&out[j]);
				i++;
				break;
			case 'x':
				fvwm_msg(OLD, "expand_vars",
					"Use $[vp.x] instead of $x");
				sprintf(&out[j], "%d", Scr.Vx);
				j += strlen(&out[j]);
				i++;
				break;
			case 'y':
				fvwm_msg(OLD, "expand_vars",
					"Use $[vp.y] instead of $y");
				sprintf(&out[j], "%d", Scr.Vy);
				j += strlen(&out[j]);
				i++;
				break;

			case 'c':
			case 'r':
			case 'n':
				if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
				{
					switch(input[i + 1])
					{
					case 'c':
						fvwm_msg(OLD, "expand_vars",
							"Use $[w.class] "
							"instead of $c");
						if (fw->class.res_class &&
						    fw->class.res_class[0])
						{
							string = fw->class.
								res_class;
						}
						break;
					case 'r':
						fvwm_msg(OLD, "expand_vars",
							"Use $[w.resource] "
							"instead of $r");
						if (fw->class.res_name &&
						    fw->class.res_name[0])
						{
							string = fw->class.
								res_name;
						}
						break;
					case 'n':
						fvwm_msg(OLD, "expand_vars",
							"Use $[w.name] "
							"instead of $n");
						if (fw->name.name &&
						    fw->name.name[0])
						{
							string = fw->name.name;
						}
						break;
					}
				}
				is_string = True;
				break;
			case 'v':
				fvwm_msg(OLD, "expand_vars",
					"Use $[version.line] instead of $v");
				sprintf(&out[j], "%s", (Fvwm_VersionInfo) ?
					Fvwm_VersionInfo : "");
				j += strlen(&out[j]);
				i++;
				break;
			case '$':
				out[j++] = '$';
				i++;
				break;
			default:
				out[j++] = input[i];
				break;
			} /* switch */
			if (is_string && string)
			{
				j = QuoteString(&out[j], string) - out;
				string = NULL;
				is_string = False;
				i++;
			}
			else if (is_string)
			{
				out[j++] = '$';
				is_string = False;
			}
		} /* if '$' */
		else
		{
			out[j++] = input[i];
		}
		i++;
	}
	out[j] = 0;

	return out;
}
