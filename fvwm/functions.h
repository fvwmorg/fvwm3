#ifndef _FUNCTIONS_
#define _FUNCTIONS_

struct FvwmFunction;               /* forward declaration */

typedef struct FunctionItem
{
  struct FvwmFunction *func;       /* the menu this item is in */
  struct FunctionItem *next_item;  /* next function item */
  char condition;                  /* the character string displayed on left*/
  char *action;                    /* action to be performed */
  short type;                      /* type of built in function */
  Bool f_needs_window;
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
  Bool func_needs_window;
};

#define FUNC_NO_WINDOW False
#define FUNC_NEEDS_WINDOW True

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

void find_func_type(char *action, short *func_type, Bool *func_needs_window);
FvwmFunction *FindFunction(char *function_name);
extern FvwmFunction *NewFvwmFunction(char *name);
void DestroyFunction(FvwmFunction *func);
extern int DeferExecution(XEvent *, Window *,FvwmWindow **, unsigned long *,
			  int, int);
void ExecuteFunction(char *Action, FvwmWindow *tmp_win, XEvent *eventp,
		     unsigned long context, int Module,
		     expand_command_type expand_cmd);
void AddToFunction(FvwmFunction *func, char *action);

#endif /* _FUNCTIONS_ */
