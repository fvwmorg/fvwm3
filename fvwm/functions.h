#ifndef _FUNCTION_
#define _FUNCTION_

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
#define ONE_AND_A_HALF_CLICKS 'o'

#endif /* _FUNCTION_ */
