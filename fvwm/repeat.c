#include "config.h"

#include <stdio.h>

#include "repeat.h"
#include "functions.h"

typedef struct
{
  char *start;
  char *end;
} double_ended_string;

/*
static struct
{
  double_ended_string string;
  double_ended_string old;
  double_ended_string builtin;
  double_ended_string function;
  double_ended_string top_function;
  double_ended_string module;
  double_ended_string menu;
  double_ended_string popup;
  double_ended_string menu_or_popup;
  int page_x;
  int page_y;
  int desk;
  FvwmWindow *fvwm_window;
} last;
*/


/* please ignore unused variables beginning with 'repeat_' for now. Thanks,
 * Dominik */
char *repeat_last_function = NULL;
char *repeat_last_complex_function = NULL;
char *repeat_last_builtin_function = NULL;
char *repeat_last_module = NULL;
/*
char *repeat_last_top_function = NULL;
char *repeat_last_menu = NULL;
FvwmWindow *repeat_last_fvwm_window = NULL;
*/


void update_last_string(char **pdest, char **pdest2, char *src, Bool no_store)
{
  if (no_store || *pdest == src)
    return;
  if (*pdest)
    free(*pdest);
  *pdest = strdup(src);
  *pdest2 = *pdest;
  return;
}


void repeat_function(F_CMD_ARGS)
{
  int index;
  char *optlist[] = {
    "function",
    "complex",
    "builtin",
    "module",
    "top",
    NULL
  };

  GetNextTokenIndex(action, optlist, 0, &index);
  switch (index)
  {
  case 1: /* complex */
fprintf(stderr,"repeating complex %s\n",repeat_last_complex_function);
    ExecuteFunction(repeat_last_complex_function, tmp_win, eventp, context,
		    *Module, DONT_EXPAND_COMMAND);
    break;
  case 2: /* builtin */
fprintf(stderr,"repeating builtin %s\n",repeat_last_builtin_function);
    ExecuteFunction(repeat_last_builtin_function, tmp_win, eventp, context,
		    *Module, DONT_EXPAND_COMMAND);
    break;
  case 3: /* module */
fprintf(stderr,"repeating module %s\n",repeat_last_module);
    ExecuteFunction(repeat_last_module, tmp_win, eventp, context, *Module,
		    DONT_EXPAND_COMMAND);
    break;
  case 0: /* function */
  default:
fprintf(stderr,"repeating %s\n",repeat_last_function);
    ExecuteFunction(repeat_last_function, tmp_win, eventp, context, *Module,
		    DONT_EXPAND_COMMAND);
    break;
  }
}
