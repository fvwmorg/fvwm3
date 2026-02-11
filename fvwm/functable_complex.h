/* -*-c-*- */

#ifndef FVWM_FUNCTABLE_COMPLEX_H
#define FVWM_FUNCTABLE_COMPLEX_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef struct FunctionItem
{
	struct FvwmFunction *func;       /* the function this item is in */
	struct FunctionItem *next_item;  /* next function item */
	char condition;                  /* the character string displayed on
					  * left*/
	char *action;                    /* action to be performed */
	func_flags_t flags;
} FunctionItem;

typedef struct FvwmFunction
{
	struct FvwmFunction *next_func;  /* next in list of root menus */
	FunctionItem *first_item;        /* first item in function */
	FunctionItem *last_item;         /* last item in function */
	char *name;                      /* function name */
	int use_depth;
	int num_items;
} FvwmFunction;

/* Types of events for the FUNCTION builtin */
typedef enum
{
	CF_IMMEDIATE =      'i',
	CF_LATE_IMMEDIATE = 'j',
	CF_MOTION =         'm',
	CF_HOLD =           'h',
	CF_CLICK =          'c',
	CF_DOUBLE_CLICK =   'd',
	CF_TIMEOUT =        '-'
} cfunc_action_t;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

FvwmFunction *NewFvwmFunction(const char *name);
void DestroyFunction(FvwmFunction *func);
FvwmFunction *find_complex_function(const char *function_name);
Bool functions_is_complex_function(const char *function_name);

#endif /* FVWM_FUNCTABLE_COMPLEX_H */
