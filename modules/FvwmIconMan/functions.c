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

#include "FvwmIconMan.h"
#include "readconfig.h"
#include "xmanager.h"
#include "x.h"
#include "libs/Module.h"
#include "libs/wild.h"

static Button *get_select_button(void);

static struct {
	Button *current_button;
	Function *fp;
} function_context;

static void init_function_context(Function *func)
{
	function_context.current_button = get_select_button();
	function_context.fp = func;
}

void run_function_list(Function *func)
{
	init_function_context(func);

	while (function_context.fp)
	{
		function_context.fp->func(
			function_context.fp->numargs,
			function_context.fp->args);
		if (function_context.fp)
		{
			function_context.fp = function_context.fp->next;
		}
	}
}

static Button *get_select_button(void)
{
	if (globals.select_win)
	{
		return globals.select_win->button;
	}
	return NULL;
}

static Button *get_focus_button(void)
{
	if (globals.focus_win)
	{
		return globals.focus_win->button;
	}
	return NULL;
}

static WinManager *get_current_man(void)
{
	Button *b = function_context.current_button;

	if (globals.num_managers==1)
	{
		return globals.managers;
	}
	if (b && b->drawn_state.win)
	{
		return b->drawn_state.win->manager;
	}
	return NULL;
}

static WinData *get_current_win(void)
{
	Button *b = function_context.current_button;

	if (b && b->drawn_state.win)
	{
		return b->drawn_state.win;
	}
	return NULL;
}

static Button *get_current_button(void)
{
	return function_context.current_button;
}

static Button *button_move(ButtonValue *bv)
{
	Button *b = NULL, *cur;
	WinManager *man;
	int i;

	cur = get_current_button();

	switch (bv->base)
	{
	case NoButton:
		ConsoleMessage("gotobutton: need a button to change to\n");
		return cur;

	case SelectButton:
		b = get_select_button();
		break;

	case FocusButton:
		b = get_focus_button();
		break;

	case AbsoluteButton:
		man = get_current_man();
		if (man && man->buttons.num_windows > 0)
		{
			i = bv->offset % man->buttons.num_windows;
			if (i < 0)
			{
				i += man->buttons.num_windows;
			}
			b = man->buttons.buttons[i];
		}
		break;

	default:
		man = get_current_man();
		if (!cur)
		{
			ConsoleDebug(
				FUNCTIONS, "\tno current button, skipping\n");
			return NULL;
		}

		switch (bv->base)
		{
		case UpButton:
			b = button_above(man, cur);
			break;

		case DownButton:
			b = button_below(man, cur);
			break;

		case LeftButton:
			b = button_left(man, cur);
			break;

		case RightButton:
			b = button_right(man, cur);
			break;

		case NextButton:
			b = button_next(man, cur);
			break;

		case PrevButton:
			b = button_prev(man, cur);
			break;

		default:
			ConsoleMessage("Internal error in gotobutton\n");
			break;
		}
	}

	return b;
}

int builtin_gotobutton(int numargs, BuiltinArg *args)
{
	Button *b;
	ButtonValue *bv;

	ConsoleDebug(FUNCTIONS, "gotobutton: ");
	print_args(numargs, args);

	bv = &args[0].value.button_value;

	b = button_move(bv);

	if (b)
	{
		function_context.current_button = b;
	}

	return 0;
}

int builtin_gotomanager(int numargs, BuiltinArg *args)
{
	ButtonValue *bv;
	WinManager *man, *new;
	int i;

	ConsoleDebug(FUNCTIONS, "gotomanager: ");
	print_args(numargs, args);

	bv = &args[0].value.button_value;

	new = man = get_current_man();

	switch (bv->base)
	{
	case NoButton:
		ConsoleMessage("gotomanager: need a manager argument\n");
		return 1;

	case SelectButton:
	case FocusButton:
		ConsoleMessage(
			"gotomanager: \"select\" or \"focus\" does not specify"
			" a manager\n");
		break;

	case AbsoluteButton:
	{
		/* Now we find the manager modulo the VISIBLE managers */
		static WinManager **wa = NULL;
		int i, num_mapped, n;

		n = globals.num_managers;
		if (n)
		{
			if (wa == NULL)
			{
				wa = xmalloc(n * sizeof(WinManager *));
			}
			for (i = 0, num_mapped = 0; i < n; i++)
			{
				if (globals.managers[i].buttons.num_windows > 0
					&& globals.managers[i].window_mapped)
				{
					wa[num_mapped++] = &globals.managers[i];
				}
			}
			if (num_mapped)
			{
				i = bv->offset % num_mapped;
				if (i < 0)
				{
					i += num_mapped;
				}
				new = wa[i];
			}
			else
			{
				new = NULL;
			}
		}
	}
	break;

	case NextButton:
		if (man)
		{
			for (i = man->index + 1, new = man + 1;
				i < globals.num_managers &&
				new->buttons.num_windows == 0;
				i++, new++)
			{
				;
			}
			if (i == globals.num_managers)
			{
				new = man;
			}
		}
		break;

	case PrevButton:
		if (man)
		{
			for (i = man->index - 1, new = man - 1;
				i > -1 && new->buttons.num_windows == 0;
				i--, new--)
			{
				;
			}
			if (i == -1)
			{
				new = man;
			}
		}
		break;

	default:
		ConsoleMessage("gotomanager: bad argument\n");
		break;
	}

	if (new && new != man && new->buttons.num_windows > 0)
	{
		function_context.current_button = new->buttons.buttons[0];
	}

	return 0;
}

int builtin_select(int numargs, BuiltinArg *args)
{
	WinManager *man = get_current_man();
	if (man)
	{
		move_highlight(man, get_current_button());
		if (get_current_button())
		{
			run_binding(man, SELECT);
		}
	}
	return 0;
}

int builtin_sendcommand(int numargs, BuiltinArg *args)
{
	WinData *win = get_current_win();

	char *command, *tmp;
	Button *current_button;
	rectangle r;
	Window tmpw;

	if (!win)
	{
		return 0;
	}

	command = args[0].value.string_value;
	current_button = win->manager->select_button;

	r.x = current_button->x;
	r.y = current_button->y;
	r.width = current_button->w;
	r.height = current_button->h;
	XTranslateCoordinates(theDisplay, win->manager->theWindow, theRoot,
		r.x, r.y, &r.x, &r.y, &tmpw);

	tmp = module_expand_action(
		theDisplay, theScreen, command, &r, NULL, NULL);
	if (tmp)
	{
		command = tmp;
	}

	SendFvwmPipe(fvwm_fd, command, win->app_id);
	if (tmp)
	{
		free(tmp);
	}

	return 0;
}

int builtin_printdebug(int numargs, BuiltinArg *args)
{
	int i;

	for (i = 0; i < globals.num_managers; i++)
	{
		ConsoleDebug(FUNCTIONS, "Manager %d\n---------\n", i);
		ConsoleDebug(FUNCTIONS, "Keys:\n");
		print_bindings(globals.managers[i].bindings[KEYPRESS]);
		ConsoleDebug(FUNCTIONS, "Mice:\n");
		print_bindings(globals.managers[i].bindings[MOUSE]);
		ConsoleDebug(FUNCTIONS, "Select:\n");
		print_bindings(globals.managers[i].bindings[SELECT]);
		ConsoleDebug(FUNCTIONS, "\n");
	}

	return 0;
}

int builtin_quit(int numargs, BuiltinArg *args)
{
	ConsoleDebug(FUNCTIONS, "quit: ");
	print_args(numargs, args);
	ShutMeDown(0);
	return 0;
}

static void do_jmp(int off)
{
	int i;
	ConsoleDebug(FUNCTIONS, "jmp: %d\n", off);

	if (off < 0)
	{
		ConsoleMessage("Can't have a negative relative jump offset\n");
		return;
	}
	for (i = 0; i < off; i++)
	{
		if (function_context.fp)
		{
			function_context.fp = function_context.fp->next;
		}
	}
}

static int eval_if(ButtonValue *bv)
{
	Button *cur;
	WinManager *man;

	switch (bv->base)
	{
	case NoButton:
		ConsoleMessage("Internal error in eval_if: 1\n");
		break;

	case SelectButton:
		if (get_select_button())
		{
			return 1;
		}
		break;

	case FocusButton:
		if (get_focus_button())
		{
			return 1;
		}
		break;

	case AbsoluteButton:
		if (bv->offset != 0)
		{
			return 1;
		}
		break;

	default:
		cur = get_current_button();
		man = get_current_man();
		if (!cur || !man)
		{
			return 0;
		}

		switch (bv->base)
		{
		case UpButton:
			return (button_above(man, cur) != cur);

		case DownButton:
			return (button_below(man, cur) != cur);

		case LeftButton:
			return (button_left(man, cur) != cur);

		case RightButton:
			return (button_right(man, cur) != cur);

		case NextButton:
			return (button_next(man, cur) != cur);

		case PrevButton:
			return (button_prev(man, cur) != cur);

		default:
			ConsoleMessage("Internal error in eval_if: 2\n");
			break;
		}
	}

	return 0;
}

int builtin_bif(int numargs, BuiltinArg *args)
{
	int off = args[1].value.int_value;
	ConsoleDebug(FUNCTIONS, "bif: off = %d\n", off);

	if (eval_if(&args[0].value.button_value))
	{
		do_jmp(off);
	}

	return 0;
}

int builtin_bifn(int numargs, BuiltinArg *args)
{
	int off = args[1].value.int_value;
	ConsoleDebug(FUNCTIONS, "bifn: off = %d\n", off);

	if (eval_if(&args[0].value.button_value) == 0)
	{
		do_jmp(off);
	}

	return 0;
}

int builtin_jmp(int numargs, BuiltinArg *args)
{
	int off = args[0].value.int_value;
	ConsoleDebug(FUNCTIONS, "jmp: off = %d\n", off);

	do_jmp(off);
	return 0;
}

int builtin_ret(int numargs, BuiltinArg *args)
{
	function_context.fp = NULL;
	return 0;
}

int builtin_print(int numargs, BuiltinArg *args)
{
	char *s;

	ConsoleDebug(FUNCTIONS, "print: %s\n", args[0].value.string_value);

	s = args[0].value.string_value;
	if (strlen(s) > 250)
	{
		ConsoleMessage("String too long\n");
	}
	else
	{
		ConsoleMessage("%s\n", s);
	}

	return 0;
}

int builtin_searchforward(int numargs, BuiltinArg *args)
{
	char *s;
	Button *b, *cur;
	WinManager *man;

	s = args[0].value.string_value;

	ConsoleDebug(FUNCTIONS, "searchforward: %s\n", s);

	cur = get_current_button();
	man = get_current_man();
	b = cur;
	if (cur)
	{
		while (1)
		{
			if (cur->drawn_state.display_string && matchWildcards(
				s, cur->drawn_state.display_string))
			{
				break;
			}
			b = button_next(man, cur);
			if (b == cur)
			{
				cur = NULL;
				break;
			}
			cur = b;
		}
	}
	if (cur)
	{
		function_context.current_button = cur;
	}

	return 0;
}

int builtin_searchback(int numargs, BuiltinArg *args)
{
	char *s;
	Button *b, *cur;
	WinManager *man;

	s = args[0].value.string_value;

	ConsoleDebug(FUNCTIONS, "searchback: %s\n", s);

	cur = get_current_button();
	man = get_current_man();
	b = cur;
	if (cur)
	{
		while (1)
		{
			if (cur->drawn_state.display_string && matchWildcards(
				s, cur->drawn_state.display_string))
			{
				break;
			}
			b = button_prev(man, cur);
			if (b == cur)
			{
				cur = NULL;
				break;
			}
			cur = b;
		}
	}
	if (cur)
	{
		function_context.current_button = cur;
	}

	return 0;
}

int builtin_warp(int numargs, BuiltinArg *args)
{
	Button *cur;
	WinManager *man;
	int x, y;

	ConsoleDebug(FUNCTIONS, "warp\n");

	cur = get_current_button();
	if (cur)
	{
		man = get_current_man();
		x = cur->x + cur->w / 2;
		y = cur->y + cur->h / 2;
		FWarpPointer(
			theDisplay, None, man->theWindow, 0, 0, 0, 0, x, y);
	}

	return 0;
}

int builtin_refresh(int numargs, BuiltinArg *args)
{
	draw_managers();
	return 0;
}
