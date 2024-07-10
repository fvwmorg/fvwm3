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
#include "libs/safemalloc.h"
#include "libs/FEvent.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "cmdparser.h"
#include "functions.h"
#include "expand.h"
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
#include "libs/log.h"

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
	"monitor.",
	"shadow.cs",
	"math.",
	NULL
};

static char *function_vars[] =
{
	"cond.rc",
	"cw.height",
	"cw.width",
	"cw.x",
	"cw.y",
	"debuglog.state",
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
	"screen.count",
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
	"w.pagex",
	"w.pagey",
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
	VAR_MONITOR_,
	VAR_SHADOW_CS,
	VAR_MATH_,
} partial_extended_vars;

enum
{
	VAR_COND_RC,
	VAR_CW_HEIGHT,
	VAR_CW_WIDTH,
	VAR_CW_X,
	VAR_CW_Y,
	VAR_DEBUG_LOG_STATE,
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
	VAR_SCREEN_COUNT,
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
	VAR_W_PAGEX,
	VAR_W_PAGEY,
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

int _eae_parse_range(char *input, int *lower, int *upper)
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

	rc = _eae_parse_range(input, &lower, &upper);
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
	int csadj = 0;
	int n;
	int nn = 0;
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
	Bool use_hash = False;
	Window context_w = Scr.Root;
	FvwmWindow *fw = exc->w.fw;
	struct monitor	*m = fw ? fw->m : monitor_get_current();
	signed int len = -1;

	/* allow partial matches for *.cs, gt, ... etc. variables */
	switch ((i = GetTokenIndex(var_name, partial_function_vars, -1, &rest)))
	{
	case VAR_FG_CS:
	case VAR_BG_CS:
	case VAR_HILIGHT_CS:
	case VAR_SHADOW_CS:
	case VAR_FGSH_CS:
		if (!isdigit(*rest) || (*rest == '0' && *(rest + 1) != 0 && *(rest + 1) != '.' ))
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
			/* Check for .lightenN or .darkenN */
			csadj = 101;
			l = -1;
			if (sscanf(rest + n, ".lighten%d%n", &l, &nn) && l >= 0 && l <= 100)
			{
				csadj = l;
			}
			if (sscanf(rest + n, ".darken%d%n", &l, &nn) && l >= 0 && l <= 100)
			{
				csadj = -l;
			}
			/* Check for .hash */
			if (StrEquals(rest + n + nn,  ".hash"))
			{
				use_hash = True;
				nn += strlen(".hash");
				if (csadj == 101)
				{
					csadj = 0;
				}
			}
			if (csadj == 101)
			{
				return -1;
			}
		}
		if (*(rest + n + nn) != 0)
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
		len = pixel_to_color_string(dpy, Pcmap, pixel, target, use_hash, csadj);
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
		struct monitor	*mon = (fw && fw->m) ? fw->m : monitor_get_current();
		string = GetDesktopName(mon, cs);
		if (string == NULL)
		{
			const char *ddn = _("Desk");
			xasprintf(&allocated_string, "%s %i", ddn, cs);
			string = allocated_string;
		}
		goto GOT_STRING;
	case VAR_MONITOR_: {
		if (strcmp(rest, "count") == 0) {
			is_numeric = True;
			val = monitor_get_count();

			goto GOT_STRING;
		}

		if (strcmp(rest, "primary") == 0) {
			struct monitor *m2 = monitor_by_primary();

			if (m2 != NULL)
				string = m2->si->name;
			should_quote = False;
			goto GOT_STRING;
		}

		if (strcmp(rest, "prev_primary") == 0) {
			struct monitor *m2 = monitor_by_last_primary();

			if (m2 != NULL)
				string = m2->si->name;
			should_quote = False;
			goto GOT_STRING;
		}

		if (strcmp(rest, "current") == 0) {
			struct monitor *m2 = monitor_get_current();

			should_quote = False;
			string = m2->si->name;

			goto GOT_STRING;
		}

		if (strcmp(rest, "prev") == 0) {
			struct monitor *m2 = monitor_get_prev();

			should_quote = False;
			string = (m2 != NULL) ? m2->si->name : "";

			goto GOT_STRING;
		}

		/* We could be left with "<NAME>.?" */
		char		*m_name = NULL;
		struct monitor  *mon2 = NULL;
		char		*rest_s;
		char *p_free;
		int got_string;

		/* The first word is the monitor name:
		 *
		 * monitor.Virtual-1
		 *
		 * so scan for the first full-stop.
		 */
		rest_s = fxstrdup(rest);
		p_free = rest_s;
		got_string = 0;
		while ((m_name = strsep(&rest_s, ".")) != NULL) {
			mon2 = monitor_resolve_name(m_name);
			if (mon2 == NULL)
			{
				free(p_free);
				return -1;
			}
			/* Skip over the monitor name. */
			rest += strlen(m_name) + 1;

			/* Match remainder to valid fields. */
			if (strcmp(rest, "name") == 0) {
				is_numeric = False;
				string = mon2->si->name;
				should_quote = False;
				got_string = 1;
				break;
			}

			if (strcmp(rest, "x") == 0) {
				is_numeric = True;
				val = mon2->si->x;
				got_string = 1;
				break;
			}

			if (strcmp(rest, "y") == 0) {
				is_numeric = True;
				val = mon2->si->y;
				got_string = 1;
				break;
			}

			if (strcmp(rest, "width") == 0) {
				is_numeric = True;
				val = mon2->si->w;
				got_string = 1;
				break;
			}

			if (strcmp(rest, "height") == 0) {
				is_numeric = True;
				val = mon2->si->h;
				got_string = 1;
				break;
			}

			if (strcmp(rest, "output") == 0) {
				is_numeric = True;
				val = (int)mon2->si->rr_output;
				got_string = 1;
				break;
			}

			if (strcmp(rest, "number") == 0) {
				is_numeric = True;
				val = (int)mon2->number;
				got_string = 1;
				fvwm_debug(__func__, "THOMAS: %s %d",
				mon2->si->name, mon2->number);
				break;
			}

			if (strcmp(rest, "desk") == 0) {
				is_numeric = True;
				val = mon2->virtual_scr.CurrentDesk;
				got_string = 1;
				break;
			}

			if (strcmp(rest, "pagex") == 0) {
				is_numeric = True;
				val = (int)(mon2->virtual_scr.Vx /
					monitor_get_all_widths());
				got_string = 1;
				break;
			}

			if (strcmp(rest, "pagey") == 0) {
				is_numeric = True;
				val = (int)(mon2->virtual_scr.Vy /
					monitor_get_all_heights());
				got_string = 1;
				break;
			}

			if (strcmp(rest, "prev_desk") == 0) {
				is_numeric = True;
				val = m->virtual_scr.prev_desk_and_page_desk;
				got_string = 1;
				break;
			}

			if (strcmp(rest, "prev_pagex") == 0) {
				is_numeric = True;
				val = (int)(m->virtual_scr.prev_page_x /
					monitor_get_all_widths());
				got_string = 1;
				break;
			}

			if (strcmp(rest, "prev_pagey") == 0) {
				is_numeric = True;
				val = (int)(m->virtual_scr.prev_page_y /
					monitor_get_all_heights());
				got_string = 1;
				break;
			}
		}
		free(p_free);
		if (got_string)
		{
			goto GOT_STRING;
		}
		break;
	}
	case VAR_MATH_:
		if (rest == NULL || rest[0] == '\0' || rest[1] != '.')
		{
			/* parsing error */
			return -1;
		}
		l = rest[0];
		rest += 2;
		if (sscanf(rest, "%d,%d%n", &x, &y, &n) < 2)
		{
			/* parsing error */
			return -1;
		}
		if (*(rest + n) != 0)
		{
			/* trailing characters */
			return -1;
		}
		switch (l) {
			case '+':
				val = x + y;
				break;
			case '-':
				val = x - y;
				break;
			case '*':
				val = x * y;
				break;
			case '/':
				val = x / y;
				break;
			case '%':
				val = x % y;
				break;
			case '^':
				val = 1;
				/* Should we limit the value of y here? */
				for (i = 0; i < y; i++)
				{
					val *= x;
				}
				break;
			default:
				/* undefined operation */
				return -1;
		}

		is_numeric = True;
		goto GOT_STRING;
	default:
		break;
	}

	/* only exact matches for all other variables */
	switch ((i = GetTokenIndex(var_name, function_vars, 0, &rest)))
	{
	case VAR_DESK_N:
		is_numeric = True;
		val = m->virtual_scr.CurrentDesk;
		break;
	case VAR_DESK_WIDTH:
		is_numeric = True;
		val = m->virtual_scr.VxMax + monitor_get_all_widths();
		break;
	case VAR_DESK_HEIGHT:
		is_numeric = True;
		val = m->virtual_scr.VyMax + monitor_get_all_heights();
		break;
	case VAR_DESK_PAGESX:
		is_numeric = True;
		val = (int)(m->virtual_scr.VxMax / monitor_get_all_widths()) + 1;
		break;
	case VAR_DESK_PAGESY:
		is_numeric = True;
		val = (int)(m->virtual_scr.VyMax / monitor_get_all_heights()) + 1;
		break;
	case VAR_VP_X:
		is_numeric = True;
		val = m->virtual_scr.Vx;
		break;
	case VAR_VP_Y:
		is_numeric = True;
		val = m->virtual_scr.Vy;
		break;
	case VAR_VP_WIDTH:
		is_numeric = True;
		val = monitor_get_all_widths();
		break;
	case VAR_VP_HEIGHT:
		is_numeric = True;
		val = monitor_get_all_heights();
		break;
	case VAR_WA_HEIGHT:
		is_numeric = True;
		val = m->Desktops->ewmh_working_area.height;
		break;
	case VAR_WA_WIDTH:
		is_numeric = True;
		val = m->Desktops->ewmh_working_area.width;
		break;
	case VAR_WA_X:
		is_numeric = True;
		val = m->Desktops->ewmh_working_area.x;
		break;
	case VAR_WA_Y:
		is_numeric = True;
		val = m->Desktops->ewmh_working_area.y;
		break;
	case VAR_DWA_HEIGHT:
		is_numeric = True;
		val = m->Desktops->ewmh_dyn_working_area.height;
		break;
	case VAR_DWA_WIDTH:
		is_numeric = True;
		val = m->Desktops->ewmh_dyn_working_area.width;
		break;
	case VAR_DWA_X:
		is_numeric = True;
		val = m->Desktops->ewmh_dyn_working_area.x;
		break;
	case VAR_DWA_Y:
		is_numeric = True;
		val = m->Desktops->ewmh_dyn_working_area.y;
		break;
	case VAR_PAGE_NX:
		is_numeric = True;
		val = (int)(m->virtual_scr.Vx / monitor_get_all_widths());
		break;
	case VAR_PAGE_NY:
		is_numeric = True;
		val = (int)(m->virtual_scr.Vy / monitor_get_all_heights());
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
			val = m->virtual_scr.CurrentDesk;
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
			is_numeric = False;

			string = (char *)fw->m->si->name;
			should_quote = False;
		}
		break;
	case VAR_W_PAGEX:
	case VAR_W_PAGEY: {
		int wx, wy, wh, ww, page_x, page_y;
		rectangle r, t;
		Window win;

		if (!fw || IS_EWMH_DESKTOP(FW_W(fw))) {
			return -1;
		}

		win = FW_W_FRAME(fw);
		if (!XGetGeometry(dpy, win, &JunkRoot, &wx, &wy,
			(unsigned int*)&ww,
			(unsigned int*)&wh,
			(unsigned int*)&JunkBW,
			(unsigned int*)&JunkDepth)) {
			return -1;
		}
		r.x = wx;
		r.y = wy;
		r.width = ww;
		r.height = wh;

		get_absolute_geometry(fw, &t, &r);
		get_page_offset_rectangle(fw, &page_x, &page_y, &t);

		if (i == VAR_W_PAGEX)
			val = page_x / monitor_get_all_widths();
		else
			val = page_y / monitor_get_all_heights();
		is_numeric = True;
		should_quote = False;
		break;
	}
	case VAR_SCREEN:
		is_numeric = False;
		val = Scr.screen;
		break;
	case VAR_SCREEN_COUNT:
		is_numeric = True;
		val = monitor_get_count();
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
		FQueryPointer(dpy, context_w, &JunkRoot, &JunkChild,
				  &JunkX, &JunkY, &x, &y, &JunkMask);
		val = (is_x) ? x : y;
		break;
	case VAR_POINTER_SCREEN:
		FQueryPointer(dpy, context_w, &JunkRoot, &JunkChild,
				&JunkX, &JunkY, &x, &y, &JunkMask);
		string = FScreenOfPointerXY(x, y);
		should_quote = False;
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
	case VAR_DEBUG_LOG_STATE:
		is_numeric = True;
		val = lib_log_level;
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
		quoted_string = fxmalloc(len * 2 + 3);
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
	char *input, cmdparser_context_t *pc, Bool addto, Bool ismod,
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
	struct monitor	*mon = fw ? fw->m : monitor_get_current();
	char *args_string = (pc) ? pc->all_pos_args_string : NULL;

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
							var, pc, addto, ismod,
							cond_rc, exc);
					}
					xlen = expand_args_extended(
						var, args_string, NULL);
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
			{
				char *s;

				if (input[i + 1] == '*')
				{
					s = args_string;
				}
				else
				{
					n = input[i + 1] - '0';
					s = (pc) ? pc->pos_arg_tokens[n] : 0;
				}
				n = input[i + 1] - '0';
				if (s != NULL)
				{
					l2 += strlen(s) - 2;
					i++;
				}
				break;
			}
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
	out = fxmalloc(l2 + 1);
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
							var, pc, addto, ismod,
							cond_rc, exc);
					}
					xlen = expand_args_extended(
						var, args_string, &out[j]);
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
			{
				char *s;

				if (input[i + 1] == '*')
				{
					s = args_string;
				}
				else
				{
					n = input[i + 1] - '0';
					s = (pc) ? pc->pos_arg_tokens[n] : NULL;
				}
				if (s != NULL)
				{
					for (k = 0; s[k]; k++)
					{
						out[j++] = s[k];
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
			}
			case '.':
				string = get_current_read_dir();
				is_string = True;
				break;
			case 'w':
				if (fw && !IS_EWMH_DESKTOP(FW_W(fw)))
				{
					fvwm_debug(__func__,
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
				fvwm_debug(__func__,
					   "Use $[desk.n] instead of $d");
				sprintf(&out[j], "%d", mon->virtual_scr.CurrentDesk);
				j += strlen(&out[j]);
				i++;
				break;
			case 'x':
				fvwm_debug(__func__,
					   "Use $[vp.x] instead of $x");
				sprintf(&out[j], "%d", mon->virtual_scr.Vx);
				j += strlen(&out[j]);
				i++;
				break;
			case 'y':
				fvwm_debug(__func__,
					   "Use $[vp.y] instead of $y");
				sprintf(&out[j], "%d", mon->virtual_scr.Vy);
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
						fvwm_debug(__func__,
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
						fvwm_debug(__func__,
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
						fvwm_debug(__func__,
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
				fvwm_debug(__func__,
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
