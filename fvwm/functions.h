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

#ifndef _FUNCTIONS_
#define _FUNCTIONS_

struct FvwmFunction;               /* forward declaration */

typedef struct FunctionItem
{
  struct FvwmFunction *func;       /* the function this item is in */
  struct FunctionItem *next_item;  /* next function item */
  char condition;                  /* the character string displayed on left*/
  char *action;                    /* action to be performed */
  short type;                      /* type of built in function */
  unsigned char flags;
} FunctionItem;

typedef struct FvwmFunction
{
  struct FvwmFunction *next_func;  /* next in list of root menus */
  FunctionItem *first_item;        /* first item in function */
  FunctionItem *last_item;         /* last item in function */
  char *name;                      /* function name */
  unsigned int use_depth;
} FvwmFunction;

/* used for parsing commands*/
struct functions
{
  char *keyword;
#ifdef __STDC__
  void (*action)(XEvent *,Window,FvwmWindow *, unsigned long,char *, int *);
#else
  void (*action)();
#endif
  short func_type;
  unsigned char flags;
};

/* Bits for the function flag byte. */
#define FUNC_NEEDS_WINDOW 0x01
#define FUNC_DONT_REPEAT  0x02

/* Types of events for the FUNCTION builtin */
#define MOTION                'm'
#define IMMEDIATE             'i'
#define CLICK                 'c'
#define DOUBLE_CLICK          'd'

/* for fExpand parameter of ExecuteFunction */
typedef enum
{
  DONT_EXPAND_COMMAND,
  EXPAND_COMMAND
} expand_command_type;

void find_func_type(char *action, short *func_type, unsigned char *flags);
FvwmFunction *FindFunction(char *function_name);
extern FvwmFunction *NewFvwmFunction(char *name);
void DestroyFunction(FvwmFunction *func);
extern int DeferExecution(XEvent *, Window *,FvwmWindow **, unsigned long *,
			  int, int);
void ExecuteFunction(char *Action, FvwmWindow *tmp_win, XEvent *eventp,
		     unsigned long context, int Module,
		     expand_command_type expand_cmd);
void ExecuteFunctionSaveTmpWin(char *Action, FvwmWindow *tmp_win,
			       XEvent *eventp, unsigned long context,
			       int Module, expand_command_type expand_cmd);
void AddToFunction(FvwmFunction *func, char *action);

#endif /* _FUNCTIONS_ */
